"""Execute parameter persistence migration and the V2 screen parameter adapter."""
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
#include "SstDpramV2Parameter.h"
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
 static const unsigned version_words[]={0,42,50,66,68,70};
 uint8_t *image=eeprom+6*64;
 unsigned words=version_words[version],length=8+2*words;
 memset(image,0,148);image[0]=0x53;image[1]=0x54;image[2]=(uint8_t)version;image[3]=(uint8_t)length;
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
 for(unsigned i=0;i<300;++i) {
  unsigned before=eepromWrites;++g_chb_dab_status.tick_10ms;SstParameterStore_Service();
  assert(eepromWrites-before<=1);
  SstParameterStore_Service();assert(eepromWrites-before<=1);
 }
 assert(!SstParameterStore_IsBusy()&&SstParameterStore_GetResult()==SST_PARAM_OK);
}
int main(void)
{
 uint16_t saved[70],liveLegacy[8],liveHardware[16];
 Stopped();memset(eeprom,0xFF,sizeof(eeprom));
 for(unsigned i=0;i<42;++i)saved[i]=SstParameterTable_SnapshotRead((uint16_t)i);
 for(unsigned i=0;i<8;++i)saved[42+i]=SstLegacyParameters_Read((uint16_t)i);
 SstHardwareParameters_Read(saved+50);
 saved[66]=7U;saved[67]=140U;
 saved[68]=181U;saved[69]=247U;
 assert(SstParameterTable_SnapshotValidate(saved)==SST_PARAM_OK);
 assert(SstLegacyParameters_ValidateSnapshot(saved+42)==SST_PARAM_OK);
 assert(SstHardwareParameters_Validate(saved+50)==SST_PARAM_OK);
 SstHardwareParameters_Read(liveHardware);
 assert(SstParameterStore_Request(1)==SST_PARAM_STORAGE); /* Blank chip keeps defaults. */
 assert(hardwareLayoutWrites==0&&protectionWrites==0);

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
 SstHardwareParameters_Read(liveHardware);
 assert(memcmp(liveHardware,saved+50,sizeof(liveHardware))==0);
 assert(hardwareLayoutWrites==0&&protectionWrites==0); /* v1/v2 have no hardware segment. */

 /* Valid CRC cannot excuse cross-field-invalid legacy thresholds or modern hardware period. */
 uint16_t malformed[70];memcpy(malformed,saved,sizeof(saved));
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
 BuildImage(2,saved);eeprom[6*64+2]=4;
 assert(SstParameterStore_Request(1)==SST_PARAM_STORAGE);
 BuildImage(2,saved);eeprom[6*64+3]=109;
 assert(SstParameterStore_Request(1)==SST_PARAM_STORAGE);
 BuildImage(2,saved);assert(SstParameterStore_Request(1)==SST_PARAM_OK);

 /* v3 restores actual PM/FO/FB/AD variables through the real hardware adapter. */
 saved[50+SST_HW_PM_A1]=0x81;saved[50+SST_HW_PM_A2]=0x04;
 saved[50+SST_HW_FO_A_ENABLE]=0x0181;saved[50+SST_HW_FB_ENABLE]=0x0041;
 BuildImage(3,saved);
 eepromReadFailure=1;
 assert(SstParameterStore_Request(1)==SST_PARAM_STORAGE);
 assert(hardwareLayoutWrites==0&&protectionWrites==0);
 eepromReadFailure=0;
 memcpy(malformed,saved,sizeof(saved));malformed[50+SST_HW_PM_A1]=0x100;
 BuildImage(3,malformed);g_chb_dab_params.current_kp=0.0F;OC_level_SET=610;
 assert(SstParameterStore_Request(1)==SST_PARAM_RANGE);
 assert(g_chb_dab_params.current_kp==0.0F&&OC_level_SET==610);
 assert(hardwareLayoutWrites==0&&protectionWrites==0);
 memcpy(malformed,saved,sizeof(saved));malformed[50+SST_HW_FO_A_ENABLE]=0x8000;
 BuildImage(3,malformed);assert(SstParameterStore_Request(1)==SST_PARAM_RANGE);
 uint16_t error_addr;
 memcpy(malformed,saved,sizeof(saved));malformed[50+SST_HW_PM_A2]=0x40U;
 BuildImage(3,malformed);
 assert(SstDpramV2Parameter_Store(1,&error_addr)==SST_PARAM_RANGE&&error_addr==0x0236U);
 assert(g_chb_dab_params.current_kp==0.0F&&OC_level_SET==610);
 memcpy(malformed,saved,sizeof(saved));malformed[50+SST_HW_FO_B_ENABLE]=0x4000U;
 BuildImage(3,malformed);
 assert(SstDpramV2Parameter_Store(1,&error_addr)==SST_PARAM_RANGE&&error_addr==0x0233U);
 g_chb_dab_params.switch_stable_10ms=8U;g_chb_dab_params.switch_timeout_10ms=170U;
 BuildImage(3,saved);assert(SstParameterStore_Request(1)==SST_PARAM_OK);
 assert(g_chb_dab_params.switch_stable_10ms==3U&&g_chb_dab_params.switch_timeout_10ms==100U);
 assert(HA_PM_A1==0x81&&HA_PM_A2==4&&Port_FO_1_EN==0x0181&&Port_FB_EN==0x0041);
 assert(hardwareLayoutWrites==1&&protectionWrites==3);
 SstHardwareParameters_Read(liveHardware);assert(memcmp(liveHardware,saved+50,sizeof(liveHardware))==0);

 /* v4 checks timing before applying anything and saves the actual two new values. */
 memcpy(malformed,saved,sizeof(saved));malformed[66]=200U;
 BuildImage(4,malformed);
 assert(SstDpramV2Parameter_Store(1,&error_addr)==SST_PARAM_RANGE&&error_addr==0x0239U);
 BuildImage(4,saved);
 g_chb_dab_params.fan_off_count=181U;g_chb_dab_params.fan_on_count=247U;
 SstParameterStore_LoadFanThresholds();
 assert(SstParameterStore_GetResult()==SST_PARAM_OK);
 assert(g_chb_dab_params.fan_off_count==181U&&g_chb_dab_params.fan_on_count==247U);
 assert(SstDpramV2Parameter_Store(1,&error_addr)==SST_PARAM_OK&&error_addr==SST_PARAM_NO_FIELD);
 assert(g_chb_dab_params.switch_stable_10ms==7U&&g_chb_dab_params.switch_timeout_10ms==140U);
 assert(g_chb_dab_params.fan_off_count==CHB_FAN_OFF_COUNT_DEFAULT&&
        g_chb_dab_params.fan_on_count==CHB_FAN_ON_COUNT_DEFAULT);

 /* A v5 boot with full restore disabled applies only its validated fan pair. */
 BuildImage(5,saved);
 g_chb_dab_params.current_kp=0.0F;OC_level_SET=610;
 g_chb_dab_params.switch_stable_10ms=8U;g_chb_dab_params.switch_timeout_10ms=170U;
 g_chb_dab_command.fan_config_apply=0U;
 unsigned layout_before=hardwareLayoutWrites;
 unsigned protection_before=protectionWrites;
 SstParameterStore_LoadFanThresholds();
 assert(SstParameterStore_GetResult()==SST_PARAM_OK);
 assert(g_chb_dab_params.fan_off_count==181U&&g_chb_dab_params.fan_on_count==247U);
 assert(g_chb_dab_command.fan_config_apply);
 assert(g_chb_dab_params.current_kp==0.0F&&OC_level_SET==610);
 assert(g_chb_dab_params.switch_stable_10ms==8U&&g_chb_dab_params.switch_timeout_10ms==170U);
 assert(hardwareLayoutWrites==layout_before&&protectionWrites==protection_before);

 /* Corrupted or invalid v5 values must never reach the controller. */
 g_chb_dab_params.fan_off_count=170U;g_chb_dab_params.fan_on_count=235U;
 g_chb_dab_command.fan_config_apply=0U;
 eeprom[6*64+142]^=1U;
 SstParameterStore_LoadFanThresholds();
 assert(SstParameterStore_GetResult()==SST_PARAM_STORAGE&&!g_chb_dab_command.fan_config_apply);
 memcpy(malformed,saved,sizeof(saved));malformed[68]=malformed[69];
 BuildImage(5,malformed);
 assert(SstDpramV2Parameter_Store(1,&error_addr)==SST_PARAM_RANGE&&error_addr==0x023DU);
 memcpy(malformed,saved,sizeof(saved));malformed[69]=65535U;
 BuildImage(5,malformed);
 SstParameterStore_LoadFanThresholds();
 assert(SstParameterStore_GetResult()==SST_PARAM_RANGE&&!g_chb_dab_command.fan_config_apply);
 assert(g_chb_dab_params.fan_off_count==170U&&g_chb_dab_params.fan_on_count==235U);
 assert(SstDpramV2Parameter_Store(1,&error_addr)==SST_PARAM_RANGE&&error_addr==0x023EU);
 assert(g_chb_dab_params.current_kp==0.0F&&OC_level_SET==610);
 BuildImage(5,saved);
 assert(SstParameterStore_Request(1)==SST_PARAM_OK);
 assert(g_chb_dab_params.switch_stable_10ms==7U&&g_chb_dab_params.switch_timeout_10ms==140U);
 assert(g_chb_dab_params.fan_off_count==181U&&g_chb_dab_params.fan_on_count==247U);

 /* Saving remains asynchronous, at most one EEPROM byte per 10 ms. */
 unsigned before=eepromWrites;g_chb_dab_protection_enable=0;
 assert(SstParameterStore_Request(0)==SST_PARAM_OK);
 assert(SstParameterStore_IsBusy());
 assert(SstParameterStore_Request(0)==SST_PARAM_BUSY&&SstParameterStore_Request(1)==SST_PARAM_BUSY);
 FinishSave();assert(eepromWrites-before==149);
 assert(eeprom[6*64+2]==5&&eeprom[6*64+3]==148);
 g_chb_dab_protection_enable=1;HA_H_Voltage=790.0F;
 g_chb_dab_params.switch_stable_10ms=3U;g_chb_dab_params.switch_timeout_10ms=100U;
 g_chb_dab_params.fan_off_count=170U;g_chb_dab_params.fan_on_count=235U;
 assert(SstParameterStore_Request(1)==SST_PARAM_OK);
 assert(g_chb_dab_protection_enable==1&&SstLegacyParameters_Read(7)==saved[49]);
 assert(g_chb_dab_params.switch_stable_10ms==7U&&g_chb_dab_params.switch_timeout_10ms==140U);
 assert(g_chb_dab_params.fan_off_count==181U&&g_chb_dab_params.fan_on_count==247U);

 /* No ACK during readback fails the save, even if the buffer happened to match. */
 assert(SstParameterStore_Request(0)==SST_PARAM_OK);
 for(unsigned i=0;i<149;++i){++g_chb_dab_status.tick_10ms;SstParameterStore_Service();}
 eepromReadFailure=1;++g_chb_dab_status.tick_10ms;SstParameterStore_Service();
 assert(!SstParameterStore_IsBusy()&&SstParameterStore_GetResult()==SST_PARAM_STORAGE);
 eepromReadFailure=0;

 /* ACK alone cannot prove persistence: emulate write protection/no data change. */
 g_chb_dab_params.current_kp=-2.0F;eepromDiscardWrites=1;
 assert(SstParameterStore_Request(0)==SST_PARAM_OK);
 for(unsigned i=0;i<300;++i){++g_chb_dab_status.tick_10ms;SstParameterStore_Service();}
 assert(!SstParameterStore_IsBusy()&&SstParameterStore_GetResult()==SST_PARAM_STORAGE);
 eepromDiscardWrites=0;assert(SstParameterStore_Request(1)==SST_PARAM_OK);

 /* A startup/debugger race aborts persistence after invalidation; an incomplete image cannot load. */
 assert(SstParameterStore_Request(0)==SST_PARAM_OK);
 ++g_chb_dab_status.tick_10ms;SstParameterStore_Service();assert(eeprom[6*64]==0);
 g_chb_dab_command.run_enable=1;++g_chb_dab_status.tick_10ms;SstParameterStore_Service();
 assert(!SstParameterStore_IsBusy()&&SstParameterStore_GetResult()==SST_PARAM_BUSY);
 Stopped();assert(SstParameterStore_Request(1)==SST_PARAM_STORAGE);
 assert(SstParameterStore_Request(0)==SST_PARAM_OK);FinishSave();
 /* Real V2 candidate offsets, error addresses, and preservation of untouched PI. */
 uint16_t actual[SST_DPRAM_V2_PARAMETER_WORDS];
 SstDpramV2Parameter_Read(actual);
 assert(actual[0x36]==0x0481U&&actual[0x32]==0x0181U&&actual[0x39]==7U&&actual[0x3A]==140U);
 g_chb_dab_params.current_kp=-1.249166667F;
 SstDpramV2Parameter_Read(actual);
 float original_kp=g_chb_dab_params.current_kp;
 actual[0x3C]=1U;
 actual[0x36]=0x0101U;
 actual[0x37]=0x4000U;
 assert(SstDpramV2Parameter_Apply(actual,&error_addr)==SST_PARAM_RANGE&&error_addr==0x0237U);
 assert(HA_PM_A1==0x81U&&HA_PM_A2==4U&&g_chb_dab_params.current_kp==original_kp);
 actual[0x37]=0U;
 actual[0x1A]=0U;
 assert(SstDpramV2Parameter_Apply(actual,&error_addr)==SST_PARAM_RANGE&&error_addr==0x021AU);
 actual[0x1A]=1U;
 actual[0x39]=25U;actual[0x3A]=20U;
 assert(SstDpramV2Parameter_Apply(actual,&error_addr)==SST_PARAM_RANGE&&error_addr==0x0239U);
 actual[0x3A]=200U;
 PortRunFLAG.Bbits.Port_RunOrTest=1U;
 assert(SstDpramV2Parameter_Apply(actual,&error_addr)==SST_PARAM_OK&&error_addr==SST_PARAM_NO_FIELD);
 assert(HA_PM_A1==1U&&HA_PM_A2==1U&&g_chb_dab_params.current_kp==original_kp);
 assert(PortRunFLAG.Bbits.Port_RunOrTest&&PortRunFLAG.Bbits.Port_AutoOrManual);
 assert(g_chb_dab_params.switch_stable_10ms==25U&&g_chb_dab_params.switch_timeout_10ms==200U);
 g_chb_dab_status.state=CHB_STATE_RUNNING;
 assert(SstDpramV2Parameter_Apply(actual,&error_addr)==SST_PARAM_BUSY);
 puts("PASS: v1-v4 migration; v5 persistence and fan-only boot; whole-image range and CRC checks; V2 offsets, scaling, error address and stopped apply");
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
        defines = ["SST_BENCH_INPUT_SIM=0","SST_SIMULATOR_MODE=0","SST_ENABLE_DPRAM=1"]
        modules = protocol_modules(ROOT) + [APP / "10_Host/SstDpramV2Parameter.c"]
        objects = compile_modules(compiler, modules, work, defines)
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
