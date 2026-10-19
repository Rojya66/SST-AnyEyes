"""Export this delivery's verified reports without overwriting historical evidence."""
from datetime import datetime, timezone
import json
from pathlib import Path
import re

from export_low_voltage_validation import (
    ROOT, HOST_CHECKS, require, read, artifact, local_path, collect_supplementary,
)
from firmware_inputs import fingerprint


def main():
    source_hash = fingerprint(ROOT)
    matrix = read('build/control_compatibility_matrix.json')
    require(matrix['case_count'] == matrix['passed'] == 17 and matrix['failed'] == 0,
            'Seventeen passing CLI builds required')
    require(matrix['fingerprints'] == [source_hash] and
            matrix['all_same_firmware_fingerprint'] and matrix['all_sources_unchanged_during_build'],
            'Stale or changing matrix sources')
    expected = {(configuration, 'audit_' + profile + '_' + protocol)
                for configuration in ('Debug', 'Release')
                for profile in ('three_phase_high', 'single_phase_high', 'single_phase_low')
                for protocol in ('anyeyes', 'vofa')}
    expected.add(('Debug', 'audit_three_phase_default'))
    expected.update((configuration, 'audit_switch_' + profile)
                    for configuration in ('Debug', 'Release')
                    for profile in ('three_phase_high', 'single_phase_low'))
    require({(r['configuration'], r['variant']) for r in matrix['results']} == expected,
            'Unexpected build coverage')
    builds = []
    for row in matrix['results']:
        result = read(row['result_path'])
        require(result == row['result'] and row['exit_code'] == 0 and
                result['compile_failures'] == [] and result['link_status'] == 0 and
                result['firmware_input_sha256'] == source_hash and
                result['firmware_unchanged_during_build'], 'Failed or stale build: ' + row['variant'])
        folder = Path(row['result_path']).parent
        builds.append(dict(configuration=row['configuration'], variant=row['variant'],
                           defines=row['defines'], warnings=result['warnings'],
                           executable=artifact(folder / 'SST_AnyEyes.out'),
                           map=artifact(folder / 'SST_AnyEyes.map'),
                           report=artifact(row['result_path'])))

    ccs = read('build/ccs_validation/result.json')
    require(ccs['firmware_input_sha256'] == source_hash and ccs['firmware_unchanged_during_build'],
            'Stale CCS report')
    require([r['action'] for r in ccs['results']] == ['import', 'Debug', 'Release'],
            'CCS import and both configurations required')
    actions = []
    for row in ccs['results']:
        require(row['exit_code'] == 0, 'CCS action failed')
        action = row['action']
        log_path = 'build/ccs_validation/' + action + '.log'
        log = local_path(log_path).read_text(encoding='utf-8-sig')
        item = dict(action=action, log=artifact(log_path))
        if action == 'import':
            require('Importing project' in log and 'Done!' in log, 'Missing CCS import confirmation')
        else:
            require(row['clean_exit_code'] == 0 and '0 out of 1 projects have errors' in log and
                    'Finished building target:' in log and 'error #' not in log,
                    'CCS clean/build diagnostics failed')
            item.update(executable=artifact(action + '/SST_AnyEyes.out'),
                        map=artifact(action + '/SST_AnyEyes.map'),
                        warnings=len(re.findall(r'warning #', log)))
        actions.append(item)

    host = read('build/host_validation/result.json')
    required = HOST_CHECKS | {'switch_io'}
    require(host['firmware_input_sha256'] == source_hash and host['firmware_unchanged_during_checks'],
            'Stale host checks')
    require(len(host['checks']) == 21 and {r['check'] for r in host['checks']} == required,
            'Twenty-one required checks missing')
    checks = []
    for row in host['checks']:
        require(row['exit_code'] == 0, 'Failed host check: ' + row['check'])
        content = local_path(row['log']).read_text(encoding='utf-8-sig')
        require('PASS' in content, 'Missing PASS result: ' + row['check'])
        checks.append(dict(check=row['check'], log=artifact(row['log']), output=content))

    supplementary = collect_supplementary(source_hash)
    for value in supplementary.values():
        if 'input_hash_scope' in value:
            value['input_hash_scope'] = 'Covered by the complete 21-check runner firmware fingerprint.'
    report = dict(
        created_utc=datetime.now(timezone.utc).isoformat(),
        delivery_tag='load-current-switch-io-20261019',
        base_commit='248cff03a3fdd95885d4eb185c1bd91ffb5f4f6c',
        firmware_input_sha256=source_hash,
        scope='Low-voltage startup/run current budgets and optional physical switch output/feedback.',
        defaults=dict(phase_a=1, low_voltage=0, bench_injection=1,
                      switch_output=0, switch_feedback=0, uart_baud=460800, vofa_period_ms=10),
        cli_build_count=17, cli_builds=builds, ccs_actions=actions,
        host_check_count=21, host_checks=checks, supplementary=supplementary,
        physical_board_validation_performed=False,
        limitations=[
            'No measured C6748 full control WCET, interrupt latency, complete stack high-water or physical I/O timing.',
            'No power-stage dynamics or hardware closed-loop stability acceptance test.',
            '50 A is a peak current-reference ceiling. Existing capacitor energy and 42 V overvoltage settings may limit continuous load.',
            'Saved defaults retain high-voltage parameters and temporary injection; real 100 Vac tests require LOW=1 and BENCH=0.',
            'Background VOFA channels are not captured simultaneously and do not constitute 10 kHz waveform recording.',
        ])
    require(source_hash == fingerprint(ROOT), 'Firmware changed during export')
    destination = ROOT / 'docs/evidence/load_current_switch_validation.json'
    destination.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n',
                           encoding='utf-8', newline='\n')
    print('PASS: 17 CLI builds, CCS import/2 clean builds, 21 host checks')
    print('Firmware SHA256:', source_hash)
    print(destination)


if __name__ == '__main__':
    main()
