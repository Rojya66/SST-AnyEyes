"""Compare custom functions/objects/types/macros to the immutable Git baseline.

Moving a declaration or full function to another file needs no exception. An
intentional interface/coordinator change must be described in the reviewed
contract file with the exact old/new lexical hashes and its validation evidence.
Fast interrupt bodies, timing macros and hardware-section assignments cannot
be waived. Never overwrites build/comment_baseline or the Git baseline tag.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import subprocess
import sys
from tree_sitter import Language, Parser
import tree_sitter_c

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from source_equivalence import (decode, inventory, source_inventory, source_paths,
                                file_inventory, lexical_hash, recover_function_spans,
                                tokens)


def inventory_preflight(root: Path) -> None:
    """Prove conditional shared-block recovery is complete and lossless."""
    fixture = '''void first(void)
{
#if LEFT_BRANCH
    if (left)
#else
    if (right)
#endif
    {
        const char *signature = "void fake(void) { }"; /* } void fake2(){} */
        consume(signature);
    }
}
void second(void) { consume("{"); }
void third(void) { /* } */ }
'''
    spans = recover_function_spans(fixture, "inventory-fixture.c")
    assert [name for _, _, name in spans] == ["first", "second", "third"]
    for start, end, name in spans:
        body = fixture[start:end]
        parsed = [item for item in inventory(body, "inventory-fixture.c")
                  if item.kind == "function" and item.name == name]
        assert len(parsed) == 1 and tokens(parsed[0].text) == tokens(body)
    first = fixture[spans[0][0]:spans[0][1]]
    assert all(directive in first for directive in ("#if LEFT_BRANCH", "#else", "#endif"))
    for name in ("left", "right"):
        changed = first.replace("(" + name + ")", "(" + name + "_changed)")
        assert lexical_hash(changed) != lexical_hash(first), "A conditional branch was omitted from hashing"
    for bad_gap in ("#define HIDDEN_GLOBAL 1", "#pragma once", "int hidden_global;"):
        bad = "void first(void) {}\n" + bad_gap + "\nvoid second(void) {}"
        try:
            recover_function_spans(bad, "inventory-gap-fixture.c")
        except AssertionError:
            pass
        else:
            raise AssertionError("Function recovery silently discarded gap entity: " + bad_gap)

    control = root / "Source/04_App/04_Control/ChbDabControl.c"
    control_paths = [control,
                     root / "Source/04_App/04_Control/ChbDabControlFast.c",
                     root / "Source/04_App/04_Control/ChbDabControlService.c"]
    entities = [item for path in control_paths
                for item in inventory(decode(path.read_bytes()), path.as_posix())
                if item.kind == "function"]
    by_name = {item.name: item for item in entities}
    # Enumerate the reviewed PI replacement and common-safety helper promotion.
    # Pin the complete predecessor set, so an unrelated missing/swallowed
    # function fails even when the overall function count happens to match.
    predecessor = git("show", "238879f:Source/04_App/04_Control/ChbDabControl.c", root=root)
    expected_names = {item.name for item in inventory(decode(predecessor), control.as_posix())
                      if item.kind == "function"} - {"Chb_SourceEquivalentPiStep"}
    safety_renames = {
        "Chb_SinglePhaseGateBlocked": "Chb_GateBlocked",
        "Chb_SinglePhaseEnablePwm": "Chb_EnablePwm",
        "Chb_SinglePhaseDisablePwm": "Chb_DisablePwm",
    }
    assert safety_renames.keys() <= expected_names, "Reviewed predecessor helper missing"
    expected_names = ((expected_names - safety_renames.keys()) |
                      set(safety_renames.values()) |
                      {"Chb_DcBusReady", "Chb_FiniteInRange",
                       "ChbDabControl_FastStepCore",
                       "ChbDabControl_ServiceCore"})
    assert len(entities) == len(by_name) and set(by_name) == expected_names, (
        "Control function inventory differs from the reviewed PI/common-safety mapping: "
        + str(sorted(set(by_name) ^ expected_names)))
    tail = ("Chb_UpdatePll", "Chb_RunOpenLoop", "Chb_GetClosedLoopIdReference",
            "Chb_RunClosedLoop", "Chb_CheckSoftwareProtection", "ChbDabControl_FastStep")
    assert all(name in by_name for name in tail), "A function following the service boundary was swallowed"

    # The former coordinator triggered tree-sitter's shared-branch error.
    # Keep that immutable recovery fixture after the current source becomes
    # syntactically simpler; current completeness is pinned by the set above.
    predecessor_text = decode(predecessor)
    predecessor_by_name = {item.name: item for item in inventory(predecessor_text, control.as_posix())
                           if item.kind == "function"}
    data = predecessor_text.encode("utf-8")
    parser = Parser(Language(tree_sitter_c.language()))
    syntax = parser.parse(data).root_node
    stack = [syntax]
    malformed = None
    while stack:
        node = stack.pop()
        if node.type == "function_definition" and node.has_error:
            node_text = data[node.start_byte:node.end_byte].decode("utf-8")
            recovered = recover_function_spans(node_text, str(control))
            if recovered:
                malformed = (node_text, recovered)
                break
        stack.extend(node.named_children)
    assert malformed is not None, "Regression fixture no longer exercises parser recovery"
    node_text, recovered = malformed
    recovered_by_name = {name: node_text[start:end] for start, end, name in recovered}
    for name in ("ChbDabControl_Service",) + tail:
        assert name in recovered_by_name
        # The current split intentionally changes the coordinator and moves
        # the fast functions; the immutable recovery fixture still proves that
        # the predecessor parser retains every branch and tail function.
        if name in predecessor_by_name:
            assert tokens(predecessor_by_name[name].text) == tokens(recovered_by_name[name]), (
                "Recovered function lost or rewrote source branch tokens: " + name)
    assert len(recovered_by_name) == len(recovered), "Recovered function spans overlap or duplicate"
    print("Source inventory preflight: PASS; conditional branches retained; all 6 tail functions recovered; invalid gaps rejected")

LOCKED = {"function:FAULTIsr", "function:Timer2Isr", "function:DPRAMIsr",
          "macro:ControlTAD", "macro:TAD_PLL"}


def git(*args: str, root: Path = ROOT) -> bytes:
    return subprocess.run(["git", *args], cwd=root, check=True,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE).stdout


def baseline_inventory(root: Path, reference: str):
    revision = git("rev-parse", reference + "^{commit}", root=root).decode().strip()
    files = git("ls-tree", "-r", "--name-only", revision, "--", "Source", root=root).decode().splitlines()
    files = [file for file in files if Path(file).suffix.lower() in (".c", ".cpp", ".h", ".hpp")
             and not any(part in ("00_Device", "01_Drive") for part in Path(file).parts)]
    result = []
    hashes = {}
    for file in files:
        contents = git("show", revision + ":" + file, root=root)
        hashes[file] = hashlib.sha256(contents).hexdigest()
        result.extend(inventory(decode(contents), file))
    return revision, hashes, result


def group(items):
    result = defaultdict(list)
    for entity in items:
        result[entity.key].append(entity)
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project", type=Path, default=ROOT)
    parser.add_argument("--baseline", default="baseline-pre-framework-2026-10-12")
    parser.add_argument("--contracts", type=Path, default=ROOT / "tests/refactor_contracts.json")
    parser.add_argument("--report", type=Path, default=ROOT / "build/refactor_equivalence.json")
    args = parser.parse_args()
    root = args.project.resolve()
    inventory_preflight(root)
    revision, baseline_files, old = baseline_inventory(root, args.baseline)
    current = source_inventory(root)
    archives = [path for directory in (root / "archive", root / "Legacy") if directory.exists()
                for path in directory.rglob("*")
                if path.is_file() and path.suffix.lower() in (".c", ".cpp", ".h", ".hpp")]
    archived = [entity for path in archives for entity in file_inventory(path, root)]
    old_groups, new_groups = group(old), group(current)
    contract_doc = json.loads(args.contracts.read_text(encoding="utf-8")) if args.contracts.exists() else {"changes": []}
    if contract_doc.get("baseline_commit") not in (None, revision):
        raise SystemExit("Contract baseline commit differs from immutable Git baseline")
    contracts = {item["key"]: item for item in contract_doc["changes"]}
    if len(contracts) != len(contract_doc["changes"]):
        raise SystemExit("Duplicate refactor contract key")
    added_reviews = contract_doc.get("added_reviews", [])
    expected_added = {"function:" + name for name in (
        "Chb_GateBlocked", "Chb_EnablePwm", "Chb_DisablePwm", "Chb_DcBusReady")}
    if len(added_reviews) != len(expected_added) or {item["key"] for item in added_reviews} != expected_added:
        raise SystemExit("Added common-safety helper contracts differ from the exact reviewed set")
    consumed = set()
    expanded_initialization = None
    if (root / "tools/control_init_equivalence.py").exists():
        from control_init_equivalence import verify
        expanded_initialization = verify()
    rows, failures = [], []
    for review in added_reviews:
        key = review["key"]
        if (key in old_groups or review.get("current_sha256s") !=
                sorted(entity.sha256 for entity in new_groups.get(key, [])) or
                not review.get("reason") or
                "common_control_safety.py" not in review.get("validation", "")):
            failures.append(key + ": added common-safety helper contract differs")
    for key, originals in sorted(old_groups.items()):
        replacements = new_groups.get(key, [])
        declaration = key.startswith(("prototype:", "extern:"))
        before = Counter(set(entity.sha256 for entity in originals)) if declaration else Counter(entity.sha256 for entity in originals)
        after = Counter(set(entity.sha256 for entity in replacements)) if declaration else Counter(entity.sha256 for entity in replacements)
        missing = before - after
        entry = dict(key=key, original=[entity.location() for entity in originals],
                     current=[entity.location() for entity in replacements])
        if not missing:
            entry["status"] = "unchanged" if sorted(entity.file for entity in originals) == sorted(
                entity.file for entity in replacements) else "moved_or_redeclared"
        elif key in ("function:Initial_Variable", "function:Variable_RST") and expanded_initialization is not None:
            proof = next(item for item in expanded_initialization['results'] if item['name'] == key.split(':', 1)[1])
            if not proof['ordered_tokens_and_directives_equal']:
                failures.append(key + ': ordered initialization helper expansion differs')
            entry.update(status="exact_ordered_initialization_expansion", proof=proof,
                         validation="tools/control_init_equivalence.py verify() executed against Git baseline in this run; complete original statement/directive stream and local types checked")
        elif key != "function:Port_IO_Out" and key not in LOCKED and not key.startswith("section:") and not (missing - Counter(
                entity.sha256 for entity in archived if entity.key == key)):
            entry.update(status="preserved_in_noncompiled_archive",
                         archive=[entity.location() for entity in archived if entity.key == key],
                         validation="Archive files are outside Source and excluded by CLI/CCS source discovery; executable original tokens preserved.")
        else:
            entry["status"] = "unreviewed_change"
            contract = contracts.get(key)
            locked = key in LOCKED or key.startswith("section:")
            if contract is not None and not locked:
                consumed.add(key)
                new_keys = contract.get("new_keys", [key])
                targets = [entity for new_key in new_keys for entity in new_groups.get(new_key, [])]
                expected_before = sorted(contract.get("baseline_sha256s", []))
                expected_after = sorted(contract.get("current_sha256s", []))
                if (expected_before == sorted(entity.sha256 for entity in originals) and
                        expected_after == sorted(entity.sha256 for entity in targets) and
                        contract.get("reason") and contract.get("validation")):
                    entry.update(status="reviewed_interface_or_organization_change",
                                 current=[entity.location() for entity in targets],
                                 reason=contract["reason"], validation=contract["validation"])
                else:
                    failures.append(key + ": contract hashes or justification do not match actual sources")
            else:
                failures.append(key + (": locked realtime/hardware entity changed" if locked else
                                       ": original executable entity changed/removed without exact reviewed contract"))
        rows.append(entry)
    for key in sorted(contracts.keys() - consumed):
        failures.append(key + ": unused contract; remove obsolete exceptions")
    new_entities = [entity for entity in current if entity.key not in old_groups]
    current_files = {path.relative_to(root).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest()
                     for path in source_paths(root) + archives}
    counts = Counter(row["status"] for row in rows)
    report = dict(baseline_reference=args.baseline, baseline_commit=revision,
                  baseline_files=baseline_files, current_files=current_files,
                  original_entities=len(old), current_entities=len(current),
                  entity_groups=dict(counts), failures=failures, entities=rows,
                  added_entities=[dict(key=entity.key, **entity.location()) for entity in new_entities],
                  method="Tree-sitter global declaration/function inventory; executable lexical hashes; includes all conditional branches",
                  limitations=["Source preservation does not prove binary timing or evaluate preprocessor branch reachability.",
                               "Reviewed changes require regression/build evidence reported separately; exact hashes prevent stale approvals."])
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print("Framework source equivalence:", len(old), "original entities;", len(current), "current entities;", dict(counts))
    for failure in failures:
        print("FAIL", failure)
    print("PASS" if not failures else "FAIL", "Report:", args.report)
    raise SystemExit(bool(failures))


if __name__ == "__main__":
    main()
