"""Exercise real AnyEyes parser, parameter map, VOFA encoder and UART arbiter with fake hardware."""
from pathlib import Path
import re, shutil, subprocess, tempfile

ROOT = Path(__file__).resolve().parents[1]
APP = ROOT / "Source/04_App"
def text(path):
    return path.read_text(encoding="utf-8-sig")
def main():
    cc=shutil.which("g++")
    if not cc: raise SystemExit("g++ required for protocol host checks")
    files=[APP/"01_FuncCode/SstParams.c", APP/"03_PcDebug/SstDebug.c",
           APP/"03_PcDebug/Comm2PcDebug.cpp", APP/"03_PcDebug/vofa.c"]
    code="\n".join(re.sub(r'^#include "(?:main.h|soc_C6748.h|hw_types.h|hw_uart.h|uart.h|gpio.h|EEProm.H)"\s*$',
                        "",text(p),flags=re.M) for p in files)
    control=text(APP/"04_Control/ChbDabControl.c")
    defaults="\n".join(re.search(r"volatile "+kind+r" "+name+r"\s*=\s*\{.*?\};",control,re.S).group(0)
                       for kind,name in [("CHB_DAB_COMMAND","g_chb_dab_command"),("CHB_DAB_PARAMETERS","g_chb_dab_params")])
    template=text(Path(__file__).with_suffix(".cpp"))
    with tempfile.TemporaryDirectory(prefix="sst_protocol_",ignore_cleanup_errors=True) as directory:
        work=Path(directory)
        for folder in ("01_FuncCode","03_PcDebug","04_Control","08_Fault","00_Config"):
            for p in (APP/folder).glob("*"):
                if p.suffix.lower() in (".h",".hpp"): shutil.copy2(p,work/p.name)
        shutil.copy2(ROOT/"Source/03_System/00_Board/SstBoard.h",work/"SstBoard.h")
        unit=work/"protocol.cpp"
        unit.write_text(template.replace("/* DEFAULTS */",defaults).replace("/* IMPLEMENTATION */",code),encoding="utf-8")
        evidence=ROOT/"build/protocol_host"
        evidence.mkdir(parents=True,exist_ok=True)
        shutil.copy2(unit,evidence/"protocol_unit.cpp")
        for name,defines in [("sim_anyeyes",[]),("remote_anyeyes",["SST_SIMULATOR_MODE=0"]),
                             ("dpram_anyeyes",["SST_SIMULATOR_MODE=0","SST_ENABLE_DPRAM=1"]),
                             ("sim_vofa",["SST_DEBUG_PROTOCOL=2"])]:
            binary=work/(name+".exe")
            command=[cc,"-std=c++11","-O2","-static","-Wall","-Wextra","-Wno-misleading-indentation",
                     *["-D"+x for x in defines],str(unit),"-o",str(binary)]
            result=subprocess.run(command,capture_output=True,text=True)
            if result.returncode: raise SystemExit(result.stdout+result.stderr)
            shutil.copy2(binary,evidence/(name+".exe"))
            try:
                result=subprocess.run([str(binary)],capture_output=True,text=True,timeout=10)
            except subprocess.TimeoutExpired as error:
                raise SystemExit(str(error)+"\n"+str(error.stdout)+"\n"+str(error.stderr))
            if result.returncode: raise SystemExit(name+" exit="+str(result.returncode)+"\n"+result.stdout+result.stderr)
            print(name+": "+result.stdout.strip())
if __name__=="__main__": main()

