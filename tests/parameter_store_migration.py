"""Exercise v1 compatibility, v2 protection/threshold persistence and whole-image validation."""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
from host_modules import protocol_modules, prepare_headers, compile_modules

ROOT = Path(__file__).resolve().parents[1]
APP = ROOT / "Source/04_App"

TEST = r'''
#include "SstParameterTable.h"
#include "SstParameterStore.h"
static uint16_t ImageCrc(const uint8_t *bytes,unsigned count)
{
 unsigned crc=0xFFFF;
 for(unsigned i=0;i<count;++i) {
  crc^=bytes[i];for(unsigned bit=0;bit<8;++bit)crc=(crc&1U)?(crc>>1)^0xA001U:crc>>1;
 }
 return (uint16_t)crc;
}
static void ImagePut(uint8_t *image,unsigned offset,uint16_t value)
{image[offset]=(uint8_t)value;image[offset+1]=(uint8_t)(value>>8);}
static void BuildImage(unsigned version,const uint16_t *snapshot)
{
 uint8_t *image=eeprom+6*64;unsigned words=version==1?42:50,length=8+2*words;
 memset(image,0,128);image[0]=0x53;image[1]=0x54;image[2]=(uint8_t)version;image[3]=(uint8_t)length;
 for(unsigned i=0;i<words;++i)ImagePut(image,6+2*i,snapshot[i]);
 ImagePut(image,length-2,ImageCrc(image,length-2));
}
static void Stopped(void)
{
 g_chb_dab_status.state=CHB_STATE_STOPPED;
 g_chb_dab_status.ac_pwm_enabled=g_chb_dab_status.dab_pwm_enabled=0;
 g_chb_dab_command.run_enable=g_chb_dab_command.dab_enable=g_chb_dab_command.fault_reset=0;
}
static void FinishSave(void)
{
 for(unsigned i=0;i<240;++i) {
  unsigned before=eepromWrites;++g_chb_dab_status.tick_10ms;SstParameterStore_Service();
  assert(eepromWrites-before<=1);
  SstParameterStore_Service();assert(eepromWrites-before<=1);
 }
 assert(!SstParameterStore_IsBusy()&&SstParameterStore_GetResult()==SST_PARAM_OK);
}
int main(void)
{
 uint16_t saved[50],liveLegacy[8];
 Stopped();memset(eeprom,0xFF,sizeof(eeprom));
 for(unsigned i=0;i<42;++i)saved[i]=SstParameterTable_SnapshotRead((uint16_t)i);
 for(unsigned i=0;i<8;++i)saved[42+i]=SstLegacyParameters_Read((uint16_t)i);
 assert(SstParameterTable_SnapshotValidate(saved)==SST_PARAM_OK);
 assert(SstLegacyParameters_ValidateSnapshot(saved+42)==SST_PARAM_OK);

 /* v1 restores only its 42 modern words; legacy threshold and protection bypass remain live. */
 BuildImage(1,saved);g_chb_dab_params.current_kp=0.0F;
 HA_H_Voltage=790.0F;g_chb_dab_protection_enable=0;
 for(unsigned i=0;i<8;++i)liveLegacy[i]=SstLegacyParameters_Read((uint16_t)i);
 assert(SstParameterStore_Request(1)==SST_PARAM_OK);
 assert(SstParameterTable_SnapshotRead(24)==saved[24]);
 for(unsigned i=0;i<8;++i)assert(SstLegacyParameters_Read((uint16_t)i)==liveLegacy[i]);
 assert(g_chb_dab_protection_enable==0);

 /* v2 restores all eight original thresholds, while run/reset and temporary bypass are never restored. */
 BuildImage(2,saved);g_chb_dab_params.current_kp=0.0F;
 OC_level_SET=610;HA_H_Voltage=790.0F;g_chb_dab_protection_enable=1;
 assert(SstParameterStore_Request(1)==SST_PARAM_OK);
 for(unsigned i=0;i<42;++i)assert(SstParameterTable_SnapshotRead((uint16_t)i)==saved[i]);
 for(unsigned i=0;i<8;++i)assert(SstLegacyParameters_Read((uint16_t)i)==saved[42+i]);
 assert(g_chb_dab_protection_enable==1&&!g_chb_dab_command.run_enable&&!g_chb_dab_command.fault_reset);

 /* Valid CRC cannot excuse cross-field-invalid legacy thresholds or modern hardware period. */
 uint16_t malformed[50];memcpy(malformed,saved,sizeof(saved));
 malformed[48]=8000;malformed[49]=7000;BuildImage(2,malformed);
 g_chb_dab_params.current_kp=0.0F;OC_level_SET=610;
 assert(SstParameterStore_Request(1)==SST_PARAM_RANGE);
 assert(g_chb_dab_params.current_kp==0.0F&&OC_level_SET==610);
 memcpy(malformed,saved,sizeof(saved));malformed[0]=(uint16_t)(saved[0]+1);BuildImage(2,malformed);
 assert(SstParameterStore_Request(1)==SST_PARAM_RANGE);
 assert(g_chb_dab_params.current_kp==0.0F&&OC_level_SET==610);

 BuildImage(2,saved);eeprom[6*64+20]^=1;
 assert(SstParameterStore_Request(1)==SST_PARAM_STORAGE);
 assert(g_chb_dab_params.current_kp==0.0F&&OC_level_SET==610);
 BuildImage(2,saved);eeprom[6*64+2]=3;
 assert(SstParameterStore_Request(1)==SST_PARAM_STORAGE);
 BuildImage(2,saved);eeprom[6*64+3]=109;
 assert(SstParameterStore_Request(1)==SST_PARAM_STORAGE);
 BuildImage(2,saved);assert(SstParameterStore_Request(1)==SST_PARAM_OK);

 /* New save is v2, at most one EEPROM byte per 10 ms, and has one shared busy owner. */
 unsigned before=eepromWrites;g_chb_dab_protection_enable=0;
 assert(SstParameterStore_Request(0)==SST_PARAM_OK);
 assert(SstParameterStore_IsBusy());
 assert(SstParameterStore_Request(0)==SST_PARAM_BUSY&&SstParameterStore_Request(1)==SST_PARAM_BUSY);
 FinishSave();assert(eepromWrites-before==109);
 assert(eeprom[6*64+2]==2&&eeprom[6*64+3]==108);
 g_chb_dab_protection_enable=1;HA_H_Voltage=790.0F;
 assert(SstParameterStore_Request(1)==SST_PARAM_OK);
 assert(g_chb_dab_protection_enable==1&&SstLegacyParameters_Read(7)==saved[49]);

 /* A startup/debugger race aborts persistence after invalidation; an incomplete image cannot load. */
 assert(SstParameterStore_Request(0)==SST_PARAM_OK);
 ++g_chb_dab_status.tick_10ms;SstParameterStore_Service();assert(eeprom[6*64]==0);
 g_chb_dab_command.run_enable=1;++g_chb_dab_status.tick_10ms;SstParameterStore_Service();
 assert(!SstParameterStore_IsBusy()&&SstParameterStore_GetResult()==SST_PARAM_BUSY);
 Stopped();assert(SstParameterStore_Request(1)==SST_PARAM_STORAGE);
 assert(SstParameterStore_Request(0)==SST_PARAM_OK);FinishSave();
 puts("PASS: v1 42-word compatibility; v2 50-word/108-byte modern+legacy persistence; no partial apply; shared busy and one-byte budget; bypass/run/reset excluded");
}
'''


def main():
    compiler = shutil.which("g++")
    if not compiler:
        raise SystemExit("g++ required for parameter persistence migration checks")
    prefix = (ROOT / "tests/protocol_regression.cpp").read_text(encoding="utf-8-sig").split("/* DEFAULTS */")[0]
    control = (APP / "04_Control/ChbDabControl.c").read_text(encoding="utf-8-sig")
    defaults = "\n".join(re.search(r"volatile " + kind + r" " + name + r"\s*=\s*\{.*?\};", control, re.S).group(0)
                         for kind, name in [("CHB_DAB_COMMAND", "g_chb_dab_command"),
                                            ("CHB_DAB_PARAMETERS", "g_chb_dab_params")])
    with tempfile.TemporaryDirectory(prefix="sst_store_migration_") as directory:
        work = Path(directory)
        prepare_headers(ROOT, work)
        unit = work / "parameter_store_migration.cpp"
        unit.write_text(prefix + "\n" + defaults + "\n" + TEST, encoding="utf-8")
        defines = ["SST_SIMULATOR_MODE=0","SST_ENABLE_DPRAM=1"]
        objects = compile_modules(compiler, protocol_modules(ROOT), work, defines)
        binary = work / "parameter_store_migration.exe"
        subprocess.run([compiler, "-std=c++11", "-O2", *["-D" + value for value in defines],
                        "-include", str(work / "host_protocol_io.h"), str(unit),
                        *[str(obj) for obj in objects], "-o", str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=15)
        print(result.stdout, end="")
        if result.returncode:
            raise SystemExit(result.stderr or f"Test exit {result.returncode}")


if __name__ == "__main__":
    main()
