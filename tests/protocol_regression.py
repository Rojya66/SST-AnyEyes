"""Exercise real AnyEyes parser, parameter map, VOFA encoder and UART arbiter with fake hardware."""
from pathlib import Path
import hashlib, json, re, shutil, subprocess, tempfile
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
        results=[]
        variants=[("sim_anyeyes",["SST_SIMULATOR_MODE=1","SST_ENABLE_DPRAM=0"]),
                             ("remote_anyeyes",["SST_SIMULATOR_MODE=0","SST_ENABLE_DPRAM=0"]),
                             ("dpram_anyeyes",["SST_SIMULATOR_MODE=0","SST_ENABLE_DPRAM=1"]),
                             ("sim_vofa",["SST_DEBUG_PROTOCOL=2","SST_SIMULATOR_MODE=1","SST_ENABLE_DPRAM=0"]),
                             ("custom_tag_anyeyes",["SST_SIMULATOR_MODE=1","SST_ENABLE_DPRAM=0",
                              "SST_PRODUCT_SERIES=201","SST_PRODUCT_PART=202","SST_HARDWARE_PLATFORM=203",
                              "SST_VERSION_BRANCH=204","SST_VERSION_RELEASE=205","SST_VERSION_BETA=206"])]
        for name,defines in variants:
            # The protocol ownership matrix is independent of the user's temporary bench build.
            defines=["SST_BENCH_INPUT_SIM=0",*defines]
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
            results.append(dict(variant=name,defines=defines,status="pass",output=result.stdout.strip(),
                                binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest()))
        # All six electronic-tag/version fields are on-wire octets, not silently truncated words.
        macro_names=["SST_PRODUCT_SERIES","SST_PRODUCT_PART","SST_HARDWARE_PLATFORM",
                     "SST_VERSION_BRANCH","SST_VERSION_RELEASE","SST_VERSION_BETA"]
        configuration_checks=[]
        for macro in macro_names:
            for value in (-1,256):
                command=[cc,"-std=c++11","-fsyntax-only","-DSST_BENCH_INPUT_SIM=0",
                         "-D"+macro+"="+str(value),"-include",str(work/"SoftVersion.h"),
                         "-x","c++","-"]
                result=subprocess.run(command,input="",capture_output=True,text=True)
                if result.returncode==0 or "#error" not in result.stderr:
                    raise SystemExit("Invalid octet was not rejected: "+macro+"="+str(value)+"\n"+result.stderr)
                configuration_checks.append(dict(macro=macro,value=value,rejected=True))
        print("PASS: six tag/version macros reject negative and >255 values (12 configurations)")
        inputs=files+[APP/"00_Version/SoftVersion.h",APP/"00_Config/ChbBuildConfig.h",
                      Path(__file__),Path(__file__).with_suffix(".cpp"),ROOT/"tests/host_modules.py",
                      ROOT/"tests/host_protocol_io.h"]
        report=dict(status="pass",variants=results,configuration_checks=configuration_checks,
                    input_sha256={p.relative_to(ROOT).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs},
                    method="Separately compile real version/parser/parameter/serial modules with fake UART/GPIO/EEPROM only.",
                    limitations="Host UART substitution does not validate RS485 electrical timing or the real AnyEyes UI.")
        (evidence/"result.json").write_text(json.dumps(report,indent=2),encoding="utf-8")
if __name__=="__main__": main()

