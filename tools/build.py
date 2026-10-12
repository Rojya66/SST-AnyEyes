"""Reproducible C6000 build, independent of Eclipse and generated makefiles."""
from pathlib import Path
import argparse
import concurrent.futures
import json
import os
import subprocess
import time
from firmware_inputs import fingerprint

ROOT = Path(__file__).resolve().parents[1]
EXCLUDED = {'usbphyGS60.c', 'pruss.c', 'mailbox.c', 'hsi2c.c', 'cppi41dma.c',
            'usbphyGS70.c', 'gpio_v2.c', 'uart_irda_cir.c', 'hs_mmcsd.c', 'cache.c'}

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--configuration', choices=['Debug', 'Release'], default='Debug')
    parser.add_argument('--variant', default='default')
    parser.add_argument('--define', action='append', default=[])
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--tool-root', default=os.environ.get('SST_CGT_ROOT',
        'E:/Applications/ti/ccs1250/ccs/tools/compiler/ti-cgt-c6000_8.3.12'))
    args = parser.parse_args()
    compiler = Path(args.tool_root) / 'bin/cl6x.exe'
    if not compiler.is_file(): parser.error('C6000 compiler not found: ' + str(compiler))
    if not args.variant.replace('-', '').replace('_', '').isalnum():
        parser.error('variant must contain letters, digits, hyphens or underscores')
    output = ROOT / 'build' / args.configuration / args.variant
    output.mkdir(parents=True, exist_ok=True)
    source = ROOT / 'Source'
    includes = [ROOT, Path(args.tool_root) / 'include']
    includes += [p for p in sorted(source.rglob('*')) if p.is_dir()]
    flags = ['-mv6740', '--define=c6748', '--diag_warning=225',
             '--diag_wrap=off', '--display_error_number']
    # Match the source project: Debug and Release both have optimization disabled.
    flags += ['-g'] if args.configuration == 'Debug' else ['--opt_level=off']
    flags += ['--include_path=' + str(p) for p in includes]
    flags += ['--define=' + d for d in args.define]
    files = [p for p in sorted(source.rglob('*')) if p.suffix.lower() in ('.c', '.cpp', '.asm')
             and p.name not in EXCLUDED]
    start = time.time()
    input_sha256 = fingerprint(ROOT)
    def compile_one(path):
        object_dir = output / path.relative_to(source).parent
        object_dir.mkdir(parents=True, exist_ok=True)
        obj = object_dir / (path.stem + '.obj')
        cmd = [str(compiler)] + flags + ['--compile_only', '--output_file=' + str(obj), str(path)]
        run = subprocess.run(cmd, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        return path, obj, run.returncode, run.stdout.decode('utf-8', errors='replace')
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = list(pool.map(compile_one, files))
    logs = []
    failures = []
    for path, obj, status, log in results:
        logs.append(str(path.relative_to(ROOT)) + '\n' + log)
        if status: failures.append(str(path.relative_to(ROOT)))
    executable = output / 'SST_AnyEyes.out'
    link_status = None
    if not failures:
        link_flags = ['-mv6740', '-z', '--map_file=' + str(output / 'SST_AnyEyes.map'),
                      '--heap_size=0x800', '--stack_size=0x800',
                      '--search_path=' + str(Path(args.tool_root) / 'lib'), '--reread_libs',
                      '--warn_sections', '--rom_model', '--display_error_number',
                      '--xml_link_info=' + str(output / 'SST_AnyEyes_linkInfo.xml'),
                      '--output_file=' + str(executable)]
        link_cmd = [str(compiler)] + link_flags + [str(r[1]) for r in results]
        link_cmd += [str(ROOT / 'C6748.cmd'), '-llibc.a']
        run = subprocess.run(link_cmd, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        link_status = run.returncode
        logs.append('LINK\n' + run.stdout.decode('utf-8', errors='replace'))
    full_log = '\n'.join(logs)
    (output / 'build.log').write_text(full_log, encoding='utf-8')
    report = dict(configuration=args.configuration, variant=args.variant, defines=args.define,
                  compiler=str(compiler), source_count=len(files), compile_failures=failures,
                  link_status=link_status, elapsed_s=round(time.time() - start, 2),
                  warnings=full_log.count('warning #'),
                  firmware_input_sha256=input_sha256,
                  firmware_unchanged_during_build=input_sha256 == fingerprint(ROOT),
                  executable=str(executable) if not failures and link_status == 0 else None)
    (output / 'build_result.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report, indent=2))
    if failures or link_status != 0:
        for entry in logs:
            if 'error #' in entry or 'undefined' in entry or 'error:' in entry:
                print(entry[-8000:])
        raise SystemExit(1)

if __name__ == '__main__': main()
