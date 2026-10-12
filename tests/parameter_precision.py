"""Verify real parameter table defaults, storage roundtrip and wire precision."""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
from host_modules import protocol_modules, prepare_headers, compile_modules, host_source


TEST = r'''
static float GetValue(const SST_PARAMETER *p) {
 return p->isWord ? (float)*(volatile unsigned short *)p->address : *(volatile float *)p->address;
}
int main(void) {
 float original[42]; uint16_t encoded[42],value; unsigned i,changed=0;
 static const unsigned commandIndices[5]={2,4,5,6,7};
 assert(sizeof(parameters)/sizeof(parameters[0])==37);
 g_chb_dab_status.state=CHB_STATE_STOPPED;
 g_chb_dab_status.ac_pwm_enabled=g_chb_dab_status.dab_pwm_enabled=0;
 g_chb_dab_command.run_enable=g_chb_dab_command.dab_enable=0;
 for(i=0;i<42;++i) {
  const SST_PARAMETER *p=i<37?&parameters[i]:&commands[commandIndices[i-37]];
  float raw=GetValue(p)*p->scale;
  assert(isfinite(raw));
  assert(raw>=(float)p->minimum-0.5F && raw<=(float)p->maximum+0.5F);
  original[i]=GetValue(p);encoded[i]=ReadEntry(p);
  if(i<37) {
   assert(SstParams_Read((uint16_t)(1+i/34),(uint16_t)(i%34),&value)==SST_PARAM_OK);
   assert(value==encoded[i]);
  }
 }
 assert(SstParams_Write(29,0,0xA55A)==SST_PARAM_OK);
 assert(SstParams_IsStorageBusy());
 assert(SstParams_RunCommand(1)==SST_PARAM_BUSY);
 assert(SstParams_Write(0,0,1)==SST_PARAM_BUSY);
 assert(SstParams_Write(0,1,1)==SST_PARAM_BUSY);
 assert(SstParams_Write(0,3,1)==SST_PARAM_BUSY);
 assert(SstParams_Write(1,24,1000)==SST_PARAM_BUSY);
 assert(SstParams_RunCommand(0)==SST_PARAM_OK);
 for(i=0;i<200;++i) {++g_chb_dab_status.tick_10ms;SstParams_Service();}
 assert(!SstParams_IsStorageBusy() && SstParams_GetStorageResult()==SST_PARAM_OK);
 assert(SstParams_Write(29,1,0xA55A)==SST_PARAM_OK);
 assert(!g_chb_dab_command.run_enable && !g_chb_dab_command.dab_enable);
 for(i=0;i<42;++i) {
  const SST_PARAMETER *p=i<37?&parameters[i]:&commands[commandIndices[i-37]];
  float restored=GetValue(p),error=fabsf(restored-original[i]);
  assert(ReadEntry(p)==encoded[i]);
  assert(error<=0.5001F/p->scale+fabsf(original[i])*0.0000002F);
  if(error>0) { ++changed;printf("entry %u: %.9g -> %.9g (delta %.9g, half-LSB %.9g)\n",
                         i,original[i],restored,restored-original[i],0.5F/p->scale); }
 }
 /* A second load is stable; rounding does not accumulate. */
 assert(SstParams_Write(29,1,0xA55A)==SST_PARAM_OK);
 for(i=0;i<37;++i) assert(ReadEntry(&parameters[i])==encoded[i]);
 g_chb_dab_command.dab_enable=1;
 assert(SstParams_Write(29,0,0xA55A)==SST_PARAM_BUSY);
 assert(SstParams_Write(1,24,1000)==SST_PARAM_BUSY);
 g_chb_dab_command.dab_enable=0;g_chb_dab_status.ac_pwm_enabled=1;
 assert(SstParams_Write(29,1,0xA55A)==SST_PARAM_BUSY);
 printf("PASS: 37 parameter + 5 command defaults representable; %u quantized within half LSB; storage interlocks and stable reload\n",changed);
 return 0;
}
'''


def main():
    project = Path(__file__).resolve().parents[1]
    app = project / "Source/04_App"
    compiler = shutil.which("g++")
    if not compiler:
        raise SystemExit("Host g++ required")
    # Reuse only the already-reviewed scalar I/O substitutes, not its scenarios.
    prefix = (project / "tests/protocol_regression.cpp").read_text(encoding="utf-8-sig").split("/* DEFAULTS */")[0]
    control = (app / "04_Control/ChbDabControl.c").read_text(encoding="utf-8-sig")
    defaults = "\n".join(re.search(r"volatile " + kind + r" " + name + r"\s*=\s*\{.*?\};",
                                  control, re.S).group(0)
                         for kind, name in (("CHB_DAB_COMMAND", "g_chb_dab_command"),
                                            ("CHB_DAB_PARAMETERS", "g_chb_dab_params")))
    table = app / "01_FuncCode/SstParameterTable.c"
    implementation = host_source(table if table.exists() else app / "01_FuncCode/SstParams.c")
    with tempfile.TemporaryDirectory(prefix="sst_param_precision_") as directory:
        work = Path(directory)
        prepare_headers(project,work)
        unit = work / "parameter_precision.cpp"
        unit.write_text("\n".join(("#include <math.h>", prefix, defaults, implementation, TEST)), encoding="utf-8")
        binary = work / "parameter_precision.exe"
        objects=compile_modules(compiler,protocol_modules(project),work,["SST_SIMULATOR_MODE=0"],
                                exclude=("SstParameterTable.c",))
        subprocess.run([compiler, "-std=c++11", "-O2", "-DSST_SIMULATOR_MODE=0",
                        "-include",str(work/"host_protocol_io.h"),str(unit),
                        *[str(obj) for obj in objects], "-o", str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10)
        print(result.stdout, end="")
        if result.returncode:
            raise SystemExit(result.stderr or f"Test exit {result.returncode}")


if __name__ == "__main__":
    main()
