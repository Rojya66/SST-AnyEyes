#include "ControlAlgorithmTypes.h"
/* 文件职责: 旧坐标变换共享结果与控制器临时状态；从原主文件迁移，符号、类型、初值及硬件地址保持。 */
#include "TransformData.h"
#include "SstControlConfig.h"

float Alpha_Trans, /* Alpha_Trans：通用坐标变换 α 轴输出，与输入电压/电流同单位。 */
    Beta_Trans; /* Beta_Trans：通用坐标变换 β 轴输出，与输入电压/电流同单位。 */

float A_Trans, /* A_Trans：通用逆变换 A 相输出，与输入量同单位。 */
    B_Trans, /* B_Trans：通用逆变换 B 相输出，与输入量同单位。 */
    C_Trans; /* C_Trans：通用逆变换 C 相输出，与输入量同单位。 */

float D_Trans, /* D_Trans：通用 Park 变换 d 轴输出，与输入量同单位。 */
    Q_Trans; /* Q_Trans：通用 Park 变换 q 轴输出，与输入量同单位。 */

#ifdef HVAC_online
XXG_PID_Controller PLL_PID_Controller, /* PLL_PID_Controller：锁相环增量 PI 的系数与上一拍输入/输出历史状态。 */
    Volt_PID_Controller, /* Volt_PID_Controller：直流电压增量 PI 的系数与上一拍输入/输出历史状态。 */
    Current_d_PID_Controller, /* Current_d_PID_Controller：d 轴电流增量 PI 的系数与上一拍输入/输出历史状态。 */
    Current_q_PID_Controller, /* Current_q_PID_Controller：q 轴电流增量 PI 的系数与上一拍输入/输出历史状态。 */
    PID_Control_buf; /* PID_Control_buf：通用增量 PI 本拍使用的控制器状态缓存。 */
#endif
