"""Exercise real AnyEyes parser, parameter map, VOFA encoder and UART arbiter with fake hardware."""
from pathlib import Path
import re, shutil, subprocess, tempfile
from host_modules import protocol_modules, prepare_headers, compile_modules

ROOT = Path(__file__).resolve().parents[1]
APP = ROOT / "Source/04_App"
def text(path):
    return path.read_text(encoding="utf-8-sig")
def main():
    cc=shutil.which("g++")
    if not cc: raise SystemExit("g++ required for protocol host checks")
    files=protocol_modules(ROOT)
    control=text(APP/"04_Control/ChbDabControl.c")
    defaults="\n".join(re.search(r"volatile "+kind+r" "+name+r"\s*=\s*\{.*?\};",control,re.S).group(0)
                       for kind,name in [("CHB_DAB_COMMAND","g_chb_dab_command"),("CHB_DAB_PARAMETERS","g_chb_dab_params")])
    template=text(Path(__file__).with_suffix(".cpp"))
    with tempfile.TemporaryDirectory(prefix="sst_protocol_",ignore_cleanup_errors=True) as directory:
        work=Path(directory)
        prepare_headers(ROOT,work)
        unit=work/"protocol.cpp"
        unit.write_text(template.replace("/* DEFAULTS */",defaults).replace("/* IMPLEMENTATION */",""),encoding="utf-8")
        evidence=ROOT/"build/protocol_host"
        evidence.mkdir(parents=True,exist_ok=True)
        shutil.copy2(unit,evidence/"protocol_unit.cpp")
        for name,defines in [("sim_anyeyes",["SST_SIMULATOR_MODE=1","SST_ENABLE_DPRAM=0"]),
                             ("remote_anyeyes",["SST_SIMULATOR_MODE=0","SST_ENABLE_DPRAM=0"]),
                             ("dpram_anyeyes",["SST_SIMULATOR_MODE=0","SST_ENABLE_DPRAM=1"]),
                             ("sim_vofa",["SST_DEBUG_PROTOCOL=2","SST_SIMULATOR_MODE=1","SST_ENABLE_DPRAM=0"])]:
            binary=work/(name+".exe")
            objects=compile_modules(cc,files,work,defines)
            command=[cc,"-std=c++11","-O2","-static","-Wall","-Wextra","-Wno-misleading-indentation",
                     *["-D"+x for x in defines],"-include",str(work/"host_protocol_io.h"),
                     str(unit),*[str(obj) for obj in objects],"-o",str(binary)]
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

