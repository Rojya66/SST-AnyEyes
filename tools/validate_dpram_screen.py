"""Build the screen delivery variants and execute applicable communication/layout regressions."""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import argparse
import hashlib
import json
import os
import subprocess
import sys
from firmware_inputs import fingerprint

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'build/dpram_screen_validation'
OUT.mkdir(exist_ok=True)
before=fingerprint(ROOT)
env=dict(os.environ)
env['PYTHONIOENCODING']='utf-8'
env['PYTHONPATH']=str(ROOT/'build/test_dependencies')+os.pathsep+env.get('PYTHONPATH','')
builds=[('Debug','default',[]),('Release','default',[]),
        ('Debug','anyeyes',['SST_DEBUG_PROTOCOL=1']),
        ('Debug','simulator',['SST_SIMULATOR_MODE=1','SST_ENABLE_DPRAM=0']),
        ('Debug','three_phase',['SST_PHASE_A_ONLY_TEST=0','SST_LOW_VOLTAGE_100V_TEST=0','SST_DEBUG_PROTOCOL=1']),
        ('Debug','switch_io',['SST_ENABLE_SWITCH_OUTPUT=1','SST_ENABLE_SWITCH_FEEDBACK=1'])]
checks=['dpram_screen','dpram_screen_layout','dpram_switch_maintenance','comment_coverage','firmware_trace','dpram_commands',
        'dpram_maintenance','parameter_precision','parameter_store_migration','protocol_regression',
        'serial_protocol_scope','pi_regression','fault_reporting','control_regression','vofa_telemetry']


def run(item):
    name,args=item
    result=subprocess.run([sys.executable,*args],cwd=ROOT,env=env,capture_output=True)
    (OUT/(name+'.log')).write_bytes(result.stdout+result.stderr)
    print(name,'PASS' if result.returncode==0 else 'FAIL',flush=True)
    return dict(name=name,exit_code=result.returncode,log=str(OUT/(name+'.log')))


jobs=[]
for config,variant,defines in builds:
    args=['tools/build.py','--configuration',config,'--variant',variant,'--jobs','3']
    for define in defines:args+=['--define',define]
    jobs.append((config+'_'+variant,args))
jobs += [(name,['tests/'+name+'.py']) for name in checks]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--retry-failed',action='store_true',help='Retry only failures with identical firmware inputs')
args=parser.parse_args()
retained=[]
retried=[]
if args.retry_failed:
    previous=json.loads((OUT/'result.json').read_text(encoding='utf-8'))
    assert previous['firmware_input_sha256']==before and previous['firmware_unchanged']
    retained=[row for row in previous['results'] if row['exit_code']==0]
    retried=[row['name'] for row in previous['results'] if row['exit_code']!=0]
    jobs=[item for item in jobs if item[0] in retried]
with ThreadPoolExecutor(max_workers=2) as pool:
    results=retained+list(pool.map(run,jobs))
artifacts=[]
for config,variant,_ in builds:
    folder=ROOT/'build'/config/variant
    for name in ['SST_DpramCompat.out','SST_DpramCompat.map','build.log','build_result.json']:
        path=folder/name
        if path.is_file():artifacts.append(dict(path=str(path),sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
report=dict(firmware_input_sha256=before,firmware_unchanged=before==fingerprint(ROOT),results=results,artifacts=artifacts,retried= retried)
(OUT/'result.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
raise SystemExit(0 if report['firmware_unchanged'] and all(r['exit_code']==0 for r in results) else 1)
