"""Compare TI-generated ISR instructions, direct calls, stack and ELF sizes.

Both sides use the saved source and the same C6000 Debug flags. Symbolic
assembly avoids treating relocated RAM addresses as algorithm changes. This
does not establish hardware interrupt latency or WCET.
"""
from pathlib import Path
import hashlib
import io
import json
import os
import re
import struct
import subprocess
import sys
import tempfile
import zipfile

ROOT=Path(__file__).resolve().parents[1]
REF='baseline-pre-framework-2026-10-12'
CGT=Path(os.environ.get('SST_CGT_ROOT','E:/Applications/ti/ccs1250/ccs/tools/compiler/ti-cgt-c6000_8.3.12'))
NAMES=['FPGAIsr','FAULTIsr','Timer2Isr','DPRAMIsr']
DEST=ROOT/'build/isr_binary_audit'
DEST.mkdir(parents=True,exist_ok=True)

def run(args,cwd):
    result=subprocess.run([str(a) for a in args],cwd=cwd,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    if result.returncode:raise RuntimeError(result.stdout.decode(errors='replace')[-8000:])
    return result.stdout

def assembly(project,source,folder):
    includes=[project,CGT/'include']+[p for p in (project/'Source').rglob('*') if p.is_dir()]
    flags=['-mv6740','--define=c6748','--diag_warning=225','--diag_wrap=off','--display_error_number','-g']
    flags+=['--include_path='+str(p) for p in includes]
    flags+=['--keep_asm','--asm_directory='+str(folder),'--compile_only','--output_file='+str(folder/'isr.obj'),str(project/source)]
    log=run([CGT/'bin/cl6x.exe']+flags,project).decode(errors='replace')
    (folder/'compile.log').write_text(log,encoding='utf-8')
    if re.search(r'warning\s+#225',log):raise AssertionError('Implicit function declaration in ISR compilation')
    return (folder/(Path(source).stem+'.asm')).read_text(encoding='utf-8')

def routine(text,name):
    start=text.index(name+':')
    end=text.index('.dwattr',text.index('DW_AT_TI_end_file',start)) if 'DW_AT_TI_end_file' in text[start:] else len(text)
    marker=text.find('\n\t.sect',start)
    if marker>=0:end=marker
    preceding=text[text.rfind('.dwattr',0,text.rfind('DW_AT_TI_max_frame_size',0,start)):start]
    body=text[start:end]
    frame=re.search(r'DW_AT_TI_max_frame_size\((0x[0-9a-fA-F]+|\d+)\)',preceding)
    clean=[]
    labels={}
    for line in body.splitlines():
        line=line.split(';',1)[0].strip()
        if not line or line.startswith('.') or re.search(r'\s\.dw\w*\b',line):continue
        for label in re.findall(r'\$C\$\w+',line):
            if label not in labels:labels[label]='$LOCAL'+str(len(labels))
        line=re.sub(r'\$C\$\w+',lambda m:labels[m[0]],line)
        clean.append(' '.join(line.split()))
    calls=re.findall(r'\bCALL(?:P)?\s+(?:\.\w+\s+)?([^,\s]+)', '\n'.join(clean))
    return dict(instructions=clean,direct_calls=calls,stack_usage=int(frame[1],0) if frame else None)

def elf_symbols(path):
    data=path.read_bytes()
    assert data[:5]==b'\x7fELF\x01' and data[5]==1
    hdr=struct.unpack_from('<HHIIIIIHHHHHH',data,16)
    shoff,shentsize,shnum=hdr[5],hdr[10],hdr[11]
    sections=[struct.unpack_from('<IIIIIIIIII',data,shoff+i*shentsize) for i in range(shnum)]
    symbols={}
    for section in sections:
        if section[1]!=2:continue
        strings=sections[section[6]];table=data[strings[4]:strings[4]+strings[5]]
        for off in range(section[4],section[4]+section[5],section[9]):
            name,value,size,info,other,index=struct.unpack_from('<IIIBBH',data,off)
            end=table.find(b'\0',name);symbol=table[name:end].decode(errors='replace')
            if symbol in NAMES:
                seg=sections[index];begin=seg[4]+value-seg[3]
                symbols[symbol]=dict(address=value,size=size,sha256=hashlib.sha256(data[begin:begin+size]).hexdigest())
    assert set(symbols)==set(NAMES),symbols
    return symbols

def main():
    archive=run(['git','archive','--format=zip',REF],ROOT)
    with tempfile.TemporaryDirectory(prefix='sst_isr_baseline_') as temp:
        project=Path(temp)
        with zipfile.ZipFile(io.BytesIO(archive)) as z:z.extractall(project)
        output=run([sys.executable,project/'tools/build.py','--jobs','6'],project)
        (DEST/'baseline_build.log').write_bytes(output)
        for name in ['baseline','current']:(DEST/name).mkdir(exist_ok=True)
        oldasm=assembly(project,'Source/03_System/01_OS/main_485test.c',DEST/'baseline')
        newasm=assembly(ROOT,'Source/03_System/02_ISR/Redef_Interrupt.c',DEST/'current')
        before=elf_symbols(project/'build/Debug/default/SST_AnyEyes.out')
        after=elf_symbols(ROOT/'build/Debug/default/SST_AnyEyes.out')
        dis=CGT/'bin/dis6x.exe'
        (DEST/'baseline/disassembly.txt').write_bytes(run([dis,'-qq','-s',project/'build/Debug/default/SST_AnyEyes.out'],project))
        (DEST/'current/disassembly.txt').write_bytes(run([dis,'-qq','-s',ROOT/'build/Debug/default/SST_AnyEyes.out'],ROOT))
        rows=[]
        for name in NAMES:
            a,b=routine(oldasm,name),routine(newasm,name)
            exact=a['instructions']==b['instructions']
            row=dict(function=name,assembly_instructions_equal=exact,direct_calls_equal=a['direct_calls']==b['direct_calls'],
                     direct_calls=b['direct_calls'],baseline_stack=a['stack_usage'],current_stack=b['stack_usage'],
                     stack_equal=a['stack_usage']==b['stack_usage'],baseline_elf=before[name],current_elf=after[name],
                     code_size_equal=before[name]['size']==after[name]['size'])
            row['passed']=all(row[k] for k in ['assembly_instructions_equal','direct_calls_equal','stack_equal','code_size_equal'])
            print(('PASS' if row['passed'] else 'FAIL'),name,'bytes',before[name]['size'],'->',after[name]['size'],'stack',a['stack_usage'],'->',b['stack_usage'])
            if not exact:
                import difflib
                (DEST/(name+'.diff')).write_text('\n'.join(difflib.unified_diff(a['instructions'],b['instructions'],fromfile='baseline',tofile='current')),encoding='utf-8')
            rows.append(row)
    report=dict(baseline=REF,compiler=str(CGT),configuration='Debug',results=rows,passed=all(r['passed'] for r in rows),
                firmware_sha256=hashlib.sha256((ROOT/'build/Debug/default/SST_AnyEyes.out').read_bytes()).hexdigest(),
                method='TI unlinked symbolic assembly instructions with generated local label names normalized and DWARF directives excluded; ELF function sizes and generated DW_AT_TI_max_frame_size',
                limitations='No board timing, nested interrupt, cache or EMIFA stall measurements; changed RAM symbol placement is expected.')
    (DEST/'result.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    raise SystemExit(0 if report['passed'] else 1)
if __name__=='__main__':main()
