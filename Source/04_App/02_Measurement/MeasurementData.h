/* 文件职责: 采样、RMS、功率和模块电压数据。数据只在所属 .c 定义，外部通过本头的必要声明访问。 */
#ifndef MEASUREMENTDATA_H_
#define MEASUREMENTDATA_H_
#include "SstControlConfig.h"
#include "SstTypes.h"
#include "SstControlConfig.h"
#include "ControlAlgorithmTypes.h"
#include "DpramRegister.h"

#ifdef __cplusplus
extern "C"
{
#endif

    extern DPRAM_RemMeaSigFloat PORT_RealTime_DATA; /* PORT_RealTime_DATA：端口遥测的浮点物理量缓存及兼容状态。 */

#ifdef HVAC_online
    extern float HA_Port_Volt_A; /* HA_Port_Volt_A：高压交流端口 A 相/相间原采样电压，单位 V。 */

#endif
#ifdef HVAC_online
    extern float HA_Port_Volt_B; /* HA_Port_Volt_B：高压交流端口 B 相/相间原采样电压，单位 V。 */

#endif
#ifdef HVAC_online
    extern float HA_Port_Volt_C; /* HA_Port_Volt_C：高压交流端口 C 相/相间原采样电压，单位 V。 */

#endif
#ifdef HVAC_online
    extern float HA_Port_Volt_AB; /* HA_Port_Volt_AB：高压交流端口 AB 相/相间原采样电压，单位 V。 */

#endif
#ifdef HVAC_online
    extern float HA_Port_Volt_BC; /* HA_Port_Volt_BC：高压交流端口 BC 相/相间原采样电压，单位 V。 */

#endif
#ifdef HVAC_online
    extern float HA_Port_Volt_CA; /* HA_Port_Volt_CA：高压交流端口 CA 相/相间原采样电压，单位 V。 */

#endif
#ifdef HVAC_online
    extern float HA_Port_Current_A; /* HA_Port_Current_A：高压交流端口 A 相/相间端口电流，单位 A。 */

#endif
#ifdef HVAC_online
    extern float HA_Port_Current_B; /* HA_Port_Current_B：高压交流端口 B 相/相间端口电流，单位 A。 */

#endif
#ifdef HVAC_online
    extern float HA_Port_Current_C; /* HA_Port_Current_C：高压交流端口 C 相/相间端口电流，单位 A。 */

#endif
#ifdef HVAC_online
    extern float HA_Grid_Current_A; /* HA_Grid_Current_A：高压交流端口 A 相/相间电网电流，单位 A。 */

#endif
#ifdef HVAC_online
    extern float HA_Grid_Current_B; /* HA_Grid_Current_B：高压交流端口 B 相/相间电网电流，单位 A。 */

#endif
#ifdef HVAC_online
    extern float HA_Grid_Current_C; /* HA_Grid_Current_C：高压交流端口 C 相/相间电网电流，单位 A。 */

#endif
#ifdef HVAC_online
    extern float
        HA_Port_Volt_AB_LATCH; /* HA_Port_Volt_AB_LATCH：高压交流端口 AB 相/相间原采样电压，单位 V；锁存采样值。 */

#endif
#ifdef HVAC_online
    extern float
        HA_Port_Volt_BC_LATCH; /* HA_Port_Volt_BC_LATCH：高压交流端口 BC 相/相间原采样电压，单位 V；锁存采样值。 */

#endif
#ifdef HVAC_online
    extern float
        HA_Port_Volt_CA_LATCH; /* HA_Port_Volt_CA_LATCH：高压交流端口 CA 相/相间原采样电压，单位 V；锁存采样值。 */

#endif
#ifdef HVAC_online
    extern float
        HA_Port_Current_A_LATCH; /* HA_Port_Current_A_LATCH：高压交流端口 A 相/相间端口电流，单位 A；锁存采样值。 */

#endif
#ifdef HVAC_online
    extern float
        HA_Port_Current_B_LATCH; /* HA_Port_Current_B_LATCH：高压交流端口 B 相/相间端口电流，单位 A；锁存采样值。 */

#endif
#ifdef HVAC_online
    extern float
        HA_Port_Current_C_LATCH; /* HA_Port_Current_C_LATCH：高压交流端口 C 相/相间端口电流，单位 A；锁存采样值。 */

#endif
#ifdef HVAC_online
    extern float
        HA_Grid_Current_A_LATCH; /* HA_Grid_Current_A_LATCH：高压交流端口 A 相/相间电网电流，单位 A；锁存采样值。 */

#endif
#ifdef HVAC_online
    extern float
        HA_Grid_Current_B_LATCH; /* HA_Grid_Current_B_LATCH：高压交流端口 B 相/相间电网电流，单位 A；锁存采样值。 */

#endif
#ifdef HVAC_online
    extern float
        HA_Grid_Current_C_LATCH; /* HA_Grid_Current_C_LATCH：高压交流端口 C 相/相间电网电流，单位 A；锁存采样值。 */

#endif
#ifdef HVAC_online
    extern float
        HA_PortUanRms_LF_1; /* HA_PortUanRms_LF_1：高压交流端口 PortUanRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */

#endif
#ifdef HVAC_online
    extern float
        HA_PortUanRms_LF_2; /* HA_PortUanRms_LF_2：高压交流端口 PortUanRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */

#endif
#ifdef HVAC_online
    extern float
        HA_PortUbnRms_LF_1; /* HA_PortUbnRms_LF_1：高压交流端口 PortUbnRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */

#endif
#ifdef HVAC_online
    extern float
        HA_PortUbnRms_LF_2; /* HA_PortUbnRms_LF_2：高压交流端口 PortUbnRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */

#endif
#ifdef HVAC_online
    extern float
        HA_PortUcnRms_LF_1; /* HA_PortUcnRms_LF_1：高压交流端口 PortUcnRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */

#endif
#ifdef HVAC_online
    extern float
        HA_PortUcnRms_LF_2; /* HA_PortUcnRms_LF_2：高压交流端口 PortUcnRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */

#endif
#ifdef HVAC_online
    extern float
        HA_PortIaRms_LF_1; /* HA_PortIaRms_LF_1：高压交流端口 PortIaRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */

#endif
#ifdef HVAC_online
    extern float
        HA_PortIaRms_LF_2; /* HA_PortIaRms_LF_2：高压交流端口 PortIaRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */

#endif
#ifdef HVAC_online
    extern float
        HA_PortIbRms_LF_1; /* HA_PortIbRms_LF_1：高压交流端口 PortIbRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */

#endif
#ifdef HVAC_online
    extern float
        HA_PortIbRms_LF_2; /* HA_PortIbRms_LF_2：高压交流端口 PortIbRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */

#endif
#ifdef HVAC_online
    extern float
        HA_PortIcRms_LF_1; /* HA_PortIcRms_LF_1：高压交流端口 PortIcRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */

#endif
#ifdef HVAC_online
    extern float
        HA_PortIcRms_LF_2; /* HA_PortIcRms_LF_2：高压交流端口 PortIcRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */

#endif
#ifdef HVAC_online
    extern float
        HA_GridIaRms_LF_1; /* HA_GridIaRms_LF_1：高压交流端口 GridIaRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */

#endif
#ifdef HVAC_online
    extern float
        HA_GridIaRms_LF_2; /* HA_GridIaRms_LF_2：高压交流端口 GridIaRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */

#endif
#ifdef HVAC_online
    extern float
        HA_GridIbRms_LF_1; /* HA_GridIbRms_LF_1：高压交流端口 GridIbRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */

#endif
#ifdef HVAC_online
    extern float
        HA_GridIbRms_LF_2; /* HA_GridIbRms_LF_2：高压交流端口 GridIbRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */

#endif
#ifdef HVAC_online
    extern float
        HA_GridIcRms_LF_1; /* HA_GridIcRms_LF_1：高压交流端口 GridIcRms 有效值低通滤波第 1 个历史状态，电压 V、电流 A。 */

#endif
#ifdef HVAC_online
    extern float
        HA_GridIcRms_LF_2; /* HA_GridIcRms_LF_2：高压交流端口 GridIcRms 有效值低通滤波第 2 个历史状态，电压 V、电流 A。 */

#endif
#ifdef HVAC_online
    extern float
        HA_Port_Volt_A_last, /* HA_Port_Volt_A_last：高压交流端口 A 相/相间原采样电压，单位 V；上一拍噪声比较缓存。 */
        HA_Port_Volt_B_last, /* HA_Port_Volt_B_last：高压交流端口 B 相/相间原采样电压，单位 V；上一拍噪声比较缓存。 */
        HA_Port_Volt_C_last; /* HA_Port_Volt_C_last：高压交流端口 C 相/相间原采样电压，单位 V；上一拍噪声比较缓存。 */

/* 本模块拥有的采样、滤波及有效值窗口数据。 */
#endif
#ifdef HVAC_online
    extern float SVG_Udc_A_Ave, /* SVG_Udc_A_Ave：A 相在线模块平均直流电压，单位 V。 */
        SVG_Udc_B_Ave, /* SVG_Udc_B_Ave：B 相在线模块平均直流电压，单位 V。 */
        SVG_Udc_C_Ave, /* SVG_Udc_C_Ave：C 相在线模块平均直流电压，单位 V。 */
        SVG_Udc_Ave; /* SVG_Udc_Ave：三相单模块直流电压的总平均值，单位 V。 */

#endif
#ifdef HVAC_online
    extern float Har_5_RMS, /* Har_5_RMS：5 次谐波电流有效值，单位 A。 */
        Har_7_RMS, /* Har_7_RMS：7 次谐波电流有效值，单位 A。 */
        Har_11_RMS; /* Har_11_RMS：11 次谐波电流有效值，单位 A。 */

#endif
#ifdef HVAC_online
    extern ushort Samplenoise; /* Samplenoise：原交流电压采样噪声判断状态。 */

#endif
#ifdef HVAC_online
    extern ushort Samplenoise_Idq; /* Samplenoise_Idq：原 dq 电流采样噪声判断状态。 */

#endif
#ifdef HVAC_online
    extern float SVG_Udc_Ave_temp, /* SVG_Udc_Ave_temp：旧直流平均电压噪声处理的本拍缓存，单位 V。 */
        SVG_Udc_Ave_temp_last; /* SVG_Udc_Ave_temp_last：旧直流平均电压噪声处理的上一拍缓存，单位 V。 */

#endif
#ifdef HVAC_online
    extern ushort Samplenoise_udc; /* Samplenoise_udc：原直流电压采样噪声判断状态。 */

#endif
#ifdef HVAC_online
    extern float HA_PortP_LF_1; /* HA_PortP_LF_1：高压交流端口有功低通滤波第 1 个历史状态。 */

#endif
#ifdef HVAC_online
    extern float HA_PortP_LF_2; /* HA_PortP_LF_2：高压交流端口有功低通滤波第 2 个历史状态。 */

#endif
#ifdef HVAC_online
    extern float HA_PortQ_LF_1; /* HA_PortQ_LF_1：高压交流端口无功低通滤波第 1 个历史状态。 */

#endif
#ifdef HVAC_online
    extern float HA_PortQ_LF_2; /* HA_PortQ_LF_2：高压交流端口无功低通滤波第 2 个历史状态。 */

#endif
#ifdef HVAC_online
    extern float LPF_5KHZ_X_Volt_A_1; /* LPF_5KHZ_X_Volt_A_1：原 5KHZ 低通滤波 X 的上一拍输入状态。 */

#endif
#ifdef HVAC_online
    extern float LPF_5KHZ_Y_Volt_A_1; /* LPF_5KHZ_Y_Volt_A_1：原 5KHZ 低通滤波 Y 的上一拍输出状态。 */

#endif
#ifdef HVAC_online
    extern float LPF_5KHZ_X_Volt_B_1; /* LPF_5KHZ_X_Volt_B_1：原 5KHZ 低通滤波 X 的上一拍输入状态。 */

#endif
#ifdef HVAC_online
    extern float LPF_5KHZ_Y_Volt_B_1; /* LPF_5KHZ_Y_Volt_B_1：原 5KHZ 低通滤波 Y 的上一拍输出状态。 */

#endif
#ifdef HVAC_online
    extern float LPF_20HZ_X_Ud_1; /* LPF_20HZ_X_Ud_1：原 20HZ 低通滤波 X 的上一拍输入状态。 */

#endif
#ifdef HVAC_online
    extern float LPF_20HZ_Y_Ud_1; /* LPF_20HZ_Y_Ud_1：原 20HZ 低通滤波 Y 的上一拍输出状态。 */

#endif
#ifdef HVAC_online
    extern float LPF_20HZ_X_Uq_1; /* LPF_20HZ_X_Uq_1：原 20HZ 低通滤波 X 的上一拍输入状态。 */

#endif
#ifdef HVAC_online
    extern float LPF_20HZ_Y_Uq_1; /* LPF_20HZ_Y_Uq_1：原 20HZ 低通滤波 Y 的上一拍输出状态。 */

#endif
#ifdef HVAC_online
    extern float HA_TotalVDC_A; /* HA_TotalVDC_A：高压交流端口 A 相/相间在线模块直流电压总和，单位 V。 */

#endif
#ifdef HVAC_online
    extern float HA_TotalVDC_B; /* HA_TotalVDC_B：高压交流端口 B 相/相间在线模块直流电压总和，单位 V。 */

#endif
#ifdef HVAC_online
    extern float HA_TotalVDC_C; /* HA_TotalVDC_C：高压交流端口 C 相/相间在线模块直流电压总和，单位 V。 */

#endif
#ifdef HVAC_online
    extern float HVAC_RealVoltage_Sum[3]; /* HVAC_RealVoltage_Sum：HVAC 实时量窗口的电压累加缓存，供原有效值计算。 */

#endif
#ifdef HVAC_online
    extern float
        HVAC_RealCurrent_Sum[3]; /* HVAC_RealCurrent_Sum：HVAC 实时量窗口的端口电流累加缓存，供原有效值计算。 */

#endif
#ifdef HVAC_online
    extern float
        HVAC_RealGridCurrent_Sum[3]; /* HVAC_RealGridCurrent_Sum：HVAC 实时量窗口的电网电流累加缓存，供原有效值计算。 */

#endif
#ifdef HVAC_online
    extern uchar HVAC_RealParameter_N; /* HVAC_RealParameter_N：HVAC 实时量窗口的采样采样窗口计数。 */

#endif
#ifdef __cplusplus
}
#endif
#endif
