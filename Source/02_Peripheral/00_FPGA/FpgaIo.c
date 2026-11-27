#include "SstControlConfig.h"
//###########################################################################
// FILE: FpgaIo.c
// FUNCTION: FPGA插件通信、模块控制和端口IO/故障状态读取。
//           原函数名、函数正文和条件编译保持；无算法改动。
//###########################################################################
#include "ControlData.h"
#include "DpramHostData.h"
#include "DpramRegister.h"
#include "FpgaRegister.h"
#include "Ft3RecorderData.h"
#include "MeasurementData.h"
#include "PortStatusData.h"
#include "ProtectionData.h"
#include "SstBoardConfig.h"
#include "SstTypes.h"

#include "FpgaIo.h"
#include "ChbBuildConfig.h"
#include "FpgaIo.h"

#ifdef HVAC_online
/*==================================================================
 * 函数功能: 触发当前端口主控与插件之间的 FPGA 读写通信
 * 输入参数: RW_fpga：原 FPGA 插件通信读/写选择值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void PortController_Communication(uchar RW_fpga)
{
    if (RW_fpga)
    {
        MainStatus_HVDC.MainStatus16 = FPGA_Interface[MainControl_PluginUnit_4 - 2].HVDC_STATUS.MainStatus16;

        //		FPGA_Interface[MainControl_PluginUnit_4-2].HVDC_DV
        //		FPGA_Interface[MainControl_PluginUnit_4-2].HVDC_DI
        //		FPGA_Interface[MainControl_PluginUnit_4-2].HVDC_DP

        MainStatus_LVDC.MainStatus16 = FPGA_Interface[MainControl_PluginUnit_4 - 2].LVDC_STATUS.MainStatus16;

        //		FPGA_Interface[MainControl_PluginUnit_4-2].LVDC_DV
        //		FPGA_Interface[MainControl_PluginUnit_4-2].LVDC_DI
        //		FPGA_Interface[MainControl_PluginUnit_4-2].LVDC_DP

        MainStatus_LVAC.MainStatus16 = FPGA_Interface[MainControl_PluginUnit_4 - 2].LVAC_STATUS.MainStatus16;

        //		FPGA_Interface[MainControl_PluginUnit_4-2].LVAC_DV
        //		FPGA_Interface[MainControl_PluginUnit_4-2].LVAC_DI
        //		FPGA_Interface[MainControl_PluginUnit_4-2].LVAC_DP
    }
    else
    {
        MainStatus_HVAC.MainStatus16 = Port_MainStatus.MainStatus16;
        FPGA_Interface[MainControl_PluginUnit_4 - 2].HVAC_STATUS.MainStatus16 = MainStatus_HVAC.MainStatus16;

        //		FPGA_Interface[MainControl_PluginUnit_4-2].HVAC_DV=
        //		FPGA_Interface[MainControl_PluginUnit_4-2].HVAC_DI=
        //		FPGA_Interface[MainControl_PluginUnit_4-2].HVAC_DP=
    }
}
#endif /* HVAC_online */
#ifdef HVAC_online
/*==================================================================
 * 函数功能: 将 Module_PM_CTL 控制字写入当前端口的模块插件
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Module_Control_Write(void)
{
#if SST_PHASE_A_ONLY_TEST
    XXG_PM_CTL inactive_phase_control; /* inactive_phase_control：B/C 相强制封波命令，保留分频、复位和协议标识。 */
#endif

    /* 只向实际配置的 UNIT 槽位发送 PM_CTL；mask=0 表示槽位未安装。 */
    if (HA_PM_A1 != 0U)
        FPGA_Interface[Com_A1_PluginUnit_5 - 2].PM_CTL.PM_CTL16 = Module_PM_CTL.PM_CTL16;
    if (HA_PM_A2 != 0U)
        FPGA_Interface[Com_A2_PluginUnit_6 - 2].PM_CTL.PM_CTL16 = Module_PM_CTL.PM_CTL16;
#if SST_PHASE_A_ONLY_TEST
    /* 所有正常使能接口最终经过此处；B/C 即使有遗留寄存器值也必须明确撤销 CHB 和 DAB 使能。 */
    inactive_phase_control.PM_CTL16 = Module_PM_CTL.PM_CTL16;
    inactive_phase_control.Bbits.SVG_EN = 0U;
    inactive_phase_control.Bbits.DAB_EN = 0U;
    FPGA_Interface[Com_B1_PluginUnit_9 - 2].PM_CTL.PM_CTL16 = inactive_phase_control.PM_CTL16;
    FPGA_Interface[Com_B2_PluginUnit_10 - 2].PM_CTL.PM_CTL16 = inactive_phase_control.PM_CTL16;
    FPGA_Interface[Com_C1_PluginUnit_13 - 2].PM_CTL.PM_CTL16 = inactive_phase_control.PM_CTL16;
    FPGA_Interface[Com_C2_PluginUnit_14 - 2].PM_CTL.PM_CTL16 = inactive_phase_control.PM_CTL16;
#else
    if (HA_PM_B1 != 0U)
        FPGA_Interface[Com_B1_PluginUnit_9 - 2].PM_CTL.PM_CTL16 = Module_PM_CTL.PM_CTL16;
    if (HA_PM_B2 != 0U)
        FPGA_Interface[Com_B2_PluginUnit_10 - 2].PM_CTL.PM_CTL16 = Module_PM_CTL.PM_CTL16;
    if (HA_PM_C1 != 0U)
        FPGA_Interface[Com_C1_PluginUnit_13 - 2].PM_CTL.PM_CTL16 = Module_PM_CTL.PM_CTL16;
    if (HA_PM_C2 != 0U)
        FPGA_Interface[Com_C2_PluginUnit_14 - 2].PM_CTL.PM_CTL16 = Module_PM_CTL.PM_CTL16;
#endif
}
#endif /* HVAC_online */
#ifdef HVAC_online
/*==================================================================
 * 函数功能: 将各相在线模块位图写入 FPGA 发送使能寄存器
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void TX_EN_Control_Write(void)
{
    FPGA_Interface[Com_A1_PluginUnit_5 - 2].TX_EN = HA_PM_A1;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].TX_EN = HA_PM_A2;
    FPGA_Interface[Com_B1_PluginUnit_9 - 2].TX_EN = HA_PM_B1;
    FPGA_Interface[Com_B2_PluginUnit_10 - 2].TX_EN = HA_PM_B2;
    FPGA_Interface[Com_C1_PluginUnit_13 - 2].TX_EN = HA_PM_C1;
    FPGA_Interface[Com_C2_PluginUnit_14 - 2].TX_EN = HA_PM_C2;
}
#endif /* HVAC_online */
#ifdef HVDC_online
/*==================================================================
 * 函数功能: 触发当前端口主控与插件之间的 FPGA 读写通信
 * 输入参数: RW_fpga：原 FPGA 插件通信读/写选择值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void PortController_Communication(uchar RW_fpga)
{
    if (RW_fpga)
    {
        MainStatus_HVAC.MainStatus16 = FPGA_Interface[MainControl_PluginUnit_4 - 2].HVAC_STATUS.MainStatus16;

        //		FPGA_Interface[MainControl_PluginUnit_4-2].HVAC_DV
        //		FPGA_Interface[MainControl_PluginUnit_4-2].HVAC_DI
        //		FPGA_Interface[MainControl_PluginUnit_4-2].HVAC_DP

        MainStatus_LVDC.MainStatus16 = FPGA_Interface[MainControl_PluginUnit_4 - 2].LVDC_STATUS.MainStatus16;

        //		FPGA_Interface[MainControl_PluginUnit_4-2].LVDC_DV
        //		FPGA_Interface[MainControl_PluginUnit_4-2].LVDC_DI
        //		FPGA_Interface[MainControl_PluginUnit_4-2].LVDC_DP

        MainStatus_LVAC.MainStatus16 = FPGA_Interface[MainControl_PluginUnit_4 - 2].LVAC_STATUS.MainStatus16;

        //		FPGA_Interface[MainControl_PluginUnit_4-2].LVAC_DV
        //		FPGA_Interface[MainControl_PluginUnit_4-2].LVAC_DI
        //		FPGA_Interface[MainControl_PluginUnit_4-2].LVAC_DP
    }
    else
    {
        MainStatus_HVDC.MainStatus16 = Port_MainStatus.MainStatus16;
        FPGA_Interface[MainControl_PluginUnit_4 - 2].HVDC_STATUS.MainStatus16 = MainStatus_HVDC.MainStatus16;

        //		FPGA_Interface[MainControl_PluginUnit_4-2].HVDC_DV=
        //		FPGA_Interface[MainControl_PluginUnit_4-2].HVDC_DI=
        //		FPGA_Interface[MainControl_PluginUnit_4-2].HVDC_DP=
    }
}
#endif /* HVDC_online */
#ifdef HVDC_online
/*==================================================================
 * 函数功能: 将 Module_PM_CTL 控制字写入当前端口的模块插件
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Module_Control_Write(void)
{
    FPGA_Interface[Com_A1_PluginUnit_5 - 2].PM_CTL.PM_CTL16 = Module_PM_CTL.PM_CTL16;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].PM_CTL.PM_CTL16 = Module_PM_CTL.PM_CTL16;
}
#endif /* HVDC_online */
#ifdef HVDC_online
/*==================================================================
 * 函数功能: 将各相在线模块位图写入 FPGA 发送使能寄存器
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void TX_EN_Control_Write(void)
{
    FPGA_Interface[Com_A1_PluginUnit_5 - 2].TX_EN = 0XFFFF;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].TX_EN = 0XFFFF;
}
#endif /* HVDC_online */
#ifdef LVDC_online
/*==================================================================
 * 函数功能: 触发当前端口主控与插件之间的 FPGA 读写通信
 * 输入参数: RW_fpga：原 FPGA 插件通信读/写选择值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void PortController_Communication(uchar RW_fpga)
{
    if (RW_fpga)
    {
        MainStatus_HVDC.MainStatus16 = FPGA_Interface[MainControl_PluginUnit_4 - 2].HVDC_STATUS.MainStatus16;

        //		FPGA_Interface[MainControl_PluginUnit_4-2].HVDC_DV
        //		FPGA_Interface[MainControl_PluginUnit_4-2].HVDC_DI
        //		FPGA_Interface[MainControl_PluginUnit_4-2].HVDC_DP

        MainStatus_HVAC.MainStatus16 = FPGA_Interface[MainControl_PluginUnit_4 - 2].HVAC_STATUS.MainStatus16;

        //		FPGA_Interface[MainControl_PluginUnit_4-2].HVAC_DV
        //		FPGA_Interface[MainControl_PluginUnit_4-2].HVAC_DI
        //		FPGA_Interface[MainControl_PluginUnit_4-2].HVAC_DP

        MainStatus_LVAC.MainStatus16 = FPGA_Interface[MainControl_PluginUnit_4 - 2].LVAC_STATUS.MainStatus16;

        //		FPGA_Interface[MainControl_PluginUnit_4-2].LVAC_DV
        //		FPGA_Interface[MainControl_PluginUnit_4-2].LVAC_DI
        //		FPGA_Interface[MainControl_PluginUnit_4-2].LVAC_DP
    }
    else
    {
        MainStatus_LVDC.MainStatus16 = Port_MainStatus.MainStatus16;
        FPGA_Interface[MainControl_PluginUnit_4 - 2].LVDC_STATUS.MainStatus16 = MainStatus_LVDC.MainStatus16;

        //		FPGA_Interface[MainControl_PluginUnit_4-2].LVDC_DV=
        //		FPGA_Interface[MainControl_PluginUnit_4-2].LVDC_DI=
        //		FPGA_Interface[MainControl_PluginUnit_4-2].LVDC_DP=
    }
}
#endif /* LVDC_online */
#ifdef LVDC_online
/*==================================================================
 * 函数功能: 将 Module_PM_CTL 控制字写入当前端口的模块插件
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Module_Control_Write(void)
{
    FPGA_Interface[Com_A1_PluginUnit_5 - 2].PM_CTL.PM_CTL16 = Module_PM_CTL.PM_CTL16;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].PM_CTL.PM_CTL16 = Module_PM_CTL.PM_CTL16;
}
#endif /* LVDC_online */
#ifdef LVDC_online
/*==================================================================
 * 函数功能: 将各相在线模块位图写入 FPGA 发送使能寄存器
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void TX_EN_Control_Write(void)
{
    FPGA_Interface[Com_A1_PluginUnit_5 - 2].TX_EN = 0XFFFF;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].TX_EN = 0XFFFF;
}
#endif /* LVDC_online */
#ifdef LVAC_online
/*==================================================================
 * 函数功能: 触发当前端口主控与插件之间的 FPGA 读写通信
 * 输入参数: RW_fpga：原 FPGA 插件通信读/写选择值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void PortController_Communication(uchar RW_fpga)
{
    if (RW_fpga)
    {
        MainStatus_HVDC.MainStatus16 = FPGA_Interface[MainControl_PluginUnit_4 - 2].HVDC_STATUS.MainStatus16;

        //		FPGA_Interface[MainControl_PluginUnit_4-2].HVDC_DV
        //		FPGA_Interface[MainControl_PluginUnit_4-2].HVDC_DI
        //		FPGA_Interface[MainControl_PluginUnit_4-2].HVDC_DP

        MainStatus_LVDC.MainStatus16 = FPGA_Interface[MainControl_PluginUnit_4 - 2].LVDC_STATUS.MainStatus16;

        //		FPGA_Interface[MainControl_PluginUnit_4-2].LVDC_DV
        //		FPGA_Interface[MainControl_PluginUnit_4-2].LVDC_DI
        //		FPGA_Interface[MainControl_PluginUnit_4-2].LVDC_DP

        MainStatus_HVAC.MainStatus16 = FPGA_Interface[MainControl_PluginUnit_4 - 2].HVAC_STATUS.MainStatus16;

        //		FPGA_Interface[MainControl_PluginUnit_4-2].HVAC_DV
        //		FPGA_Interface[MainControl_PluginUnit_4-2].HVAC_DI
        //		FPGA_Interface[MainControl_PluginUnit_4-2].HVAC_DP
    }
    else
    {
        MainStatus_LVAC.MainStatus16 = Port_MainStatus.MainStatus16;
        FPGA_Interface[MainControl_PluginUnit_4 - 2].LVAC_STATUS.MainStatus16 = MainStatus_LVAC.MainStatus16;

        //		FPGA_Interface[MainControl_PluginUnit_4-2].LVAC_DV=
        //		FPGA_Interface[MainControl_PluginUnit_4-2].LVAC_DI=
        //		FPGA_Interface[MainControl_PluginUnit_4-2].LVAC_DP=
    }
}
#endif /* LVAC_online */
#ifdef LVAC_online
/*==================================================================
 * 函数功能: 将 Module_PM_CTL 控制字写入当前端口的模块插件
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Module_Control_Write(void)
{
    FPGA_Interface[Com_A1_PluginUnit_5 - 2].PM_CTL.PM_CTL16 = Module_PM_CTL.PM_CTL16;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].PM_CTL.PM_CTL16 = Module_PM_CTL.PM_CTL16;

    //	FPGA_Interface[InvPWM_A_PluginUnit_9-2].PM_CTL.PM_CTL16=Module_PM_CTL.PM_CTL16;
    //	FPGA_Interface[InvPWM_B_PluginUnit_10-2].PM_CTL.PM_CTL16=Module_PM_CTL.PM_CTL16;
    //	FPGA_Interface[InvPWM_C_PluginUnit_11-2].PM_CTL.PM_CTL16=Module_PM_CTL.PM_CTL16;
    //	FPGA_Interface[InvPWM_N_PluginUnit_12-2].PM_CTL.PM_CTL16=Module_PM_CTL.PM_CTL16;
}
#endif /* LVAC_online */
#ifdef LVAC_online
/*==================================================================
 * 函数功能: 将各相在线模块位图写入 FPGA 发送使能寄存器
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void TX_EN_Control_Write(void)
{
    FPGA_Interface[Com_A1_PluginUnit_5 - 2].TX_EN = 0XFFFF;
    FPGA_Interface[Com_A2_PluginUnit_6 - 2].TX_EN = 0XFFFF;
}
#endif /* LVAC_online */
/*==================================================================
 * 函数功能: 按位图设置模块工频桥、DAB 桥的 PWM 使能
 * 输入参数: IndexM：模块桥类型使能位图，沿用原函数按位解释
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Module_PWMEN(uchar IndexM)
{
    if (IndexM == 0)
    {
        ControlFLAG.Bbits.SVG_PWMEN = 0;
        ControlFLAG.Bbits.DAB_PWMEN = 0;
    }
    else if (IndexM == 1)
    {
        ControlFLAG.Bbits.SVG_PWMEN = 1;
        ControlFLAG.Bbits.DAB_PWMEN = 0;
    }
    else if (IndexM == 2)
    {
        ControlFLAG.Bbits.SVG_PWMEN = 0;
        ControlFLAG.Bbits.DAB_PWMEN = 1;
    }
    else if (IndexM == 3)
    {
        ControlFLAG.Bbits.SVG_PWMEN = 1;
        ControlFLAG.Bbits.DAB_PWMEN = 1;
    }
    else
    {
    }

    Module_PM_CTL.Bbits.SVG_EN = ControlFLAG.Bbits.SVG_PWMEN;
    Module_PM_CTL.Bbits.DAB_EN = ControlFLAG.Bbits.DAB_PWMEN;

    Module_Control_Write();
}

/*==================================================================
 * 函数功能: 设置模块工频桥 PWM 使能并回写 FPGA 控制字
 * 输入参数: EN50Hz：工频桥使能，0 禁用、非零使能
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Module_50Hz_PWMEN(uchar EN50Hz)
{
    if (EN50Hz)
    {
        ControlFLAG.Bbits.SVG_PWMEN = 1;
        //		Module_PM_CTL.Bbits.SVG_RST=0;
        //		Module_PM_CTL.Bbits.HW_RST=0;
    }
    else
    {
        ControlFLAG.Bbits.SVG_PWMEN = 0;
    }

    Module_PM_CTL.Bbits.SVG_EN = ControlFLAG.Bbits.SVG_PWMEN;

    Module_Control_Write();
}

/*==================================================================
 * 函数功能: 设置模块 DAB 桥 PWM 使能并回写 FPGA 控制字
 * 输入参数: ENdab：DAB 桥使能，0 禁用、非零使能
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Module_ENdab_PWMEN(uchar ENdab)
{
    if (ENdab)
    {
        ControlFLAG.Bbits.DAB_PWMEN = 1;
        //		Module_PM_CTL.Bbits.DAB_RST=0;
        //		Module_PM_CTL.Bbits.HW_RST=0;
    }
    else
    {
        ControlFLAG.Bbits.DAB_PWMEN = 0;
    }

    Module_PM_CTL.Bbits.DAB_EN = ControlFLAG.Bbits.DAB_PWMEN;

    Module_Control_Write();
}

/*==================================================================
 * 函数功能: 设置工频桥与 DAB 桥的驱动分频控制位
 * 输入参数: DoublingValue50Hz：工频桥驱动分频字段值
 *           DoublingValueDAB：DAB 驱动分频字段值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void MOSFET_50Hz_FrequencyDoubling(uchar DoublingValue50Hz, uchar DoublingValueDAB)
{
    Module_PM_CTL.Bbits.SVG_DIV = DoublingValue50Hz;
    Module_PM_CTL.Bbits.DAB_DIV = DoublingValueDAB;
    Module_Control_Write();
}

/*==================================================================
 * 函数功能: 设置工频桥与 DAB 桥驱动复位控制位
 * 输入参数: Rst50Hz：工频桥驱动复位控制值
 *           RstDab：DAB 驱动复位控制值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void MOSFET_DriverReset(uchar Rst50Hz, uchar RstDab)
{
    if (Rst50Hz)
    {
        Module_PM_CTL.Bbits.SVG_RST = 1;
    }
    if (RstDab)
    {
        Module_PM_CTL.Bbits.DAB_RST = 1;
    }
    Module_Control_Write();
}

/*==================================================================
 * 函数功能: 设置模块硬件复位控制位
 * 输入参数: runindex：模块硬件复位控制值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Module_HW_RST(uchar runindex)
{
    Module_PM_CTL.Bbits.HW_RST = runindex;
    Module_Control_Write();
}

/*==================================================================
 * 函数功能: 设置模块风扇关闭控制位
 * 输入参数: FANdisable：模块风扇关闭控制值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Module_FAN_OFF(uchar FANdisable)
{
    if (FANdisable)
    {
        Module_PM_CTL.Bbits.FAN_OFF = 1;
    }
    else
    {
        Module_PM_CTL.Bbits.FAN_OFF = 0;
    }
    Module_Control_Write();
}

/*==================================================================
 * 函数功能: 设置两类桥的硬件关断有效控制位
 * 输入参数: valid50Hz：工频桥关断有效控制值
 *           validDab：DAB 桥关断有效控制值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Module_Shutdown_valid(uchar valid50Hz, uchar validDab)
{
    if (valid50Hz)
    {
        Module_PM_CTL.Bbits.SVG_SHUT = 1;
    }
    else
    {
        Module_PM_CTL.Bbits.SVG_SHUT = 0;
    }

    if (validDab)
    {
        Module_PM_CTL.Bbits.DAB_SHUT = 1;
    }
    else
    {
        Module_PM_CTL.Bbits.DAB_SHUT = 0;
    }

    Module_Control_Write();
}

/*==================================================================
 * 函数功能: 设置模块控制字中的测试模式字段
 * 输入参数: TestMode：模块单链测试模式字段值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Module_TestMode(uchar TestMode)
{
    if (TestMode)
    {
        Module_PM_CTL.Bbits.ONE = 1;
    }
    else
    {
        Module_PM_CTL.Bbits.ONE = 0;
    }

    Module_Control_Write();
}

/*==================================================================
 * 函数功能: 将S301输出字提交到开关量板FPGA
 * 输入参数: 无；读取Port_DOUT.Dout，bit0/1/2分别为QF1合/分脉冲及KT1保持
 * 输出参数: IO_PluginUnit_2对应的DOUT寄存器，DSP字节地址0x60000240
 * 返回值: 无
 * 执行说明: 301 FPGA已将reg_DOUT取反驱动低有效光耦，DSP写1即继电器吸合，
 *           因此直接写入，不在DSP再次取反。输出宏关闭时不写寄存器。
 *================================================================*/
void Port_IO_Out(void)
{
#if SST_ENABLE_SWITCH_OUTPUT
    /* IO_PluginUnit_2=3，数组下标1；DO1~DO5与图纸S301端子一一对应。 */
    FPGA_Interface[IO_PluginUnit_2 - 2].DOUT.SwitchINOUT16 = Port_DOUT.SwitchINOUT16;
#endif
}

/*==================================================================
 * 函数功能: 写入端口 AD 硬件保护使能位图
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Port_ADProtect_EN(void)
{
    FPGA_Interface[Analog_PluginUnit_3 - 2].AD_PROT_EN = Port_AD_PORT_EN;
}

/*==================================================================
 * 函数功能: 按光纤插件编号写入各组模块关断位图
 * 输入参数: OpticalNum：光纤插件/组编号
 *           Shutdown_1：第 1 组模块关断位图
 *           Shutdown_2：第 2 组模块关断位图
 *           Shutdown_3：第 3 组模块关断位图
 *           Shutdown_4：第 4 组模块关断位图
 *           Shutdown_5：第 5 组模块关断位图
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Port_ModuleShutdown(
    ushort OpticalNum, ushort Shutdown_1, ushort Shutdown_2, ushort Shutdown_3, ushort Shutdown_4, ushort Shutdown_5)
{
    if (OpticalNum == 0)
    {
        FPGA_Interface[Putout_A_PluginUnit_7 - 2].SHUTDOWN = Shutdown_1;
    }
    else if (OpticalNum == 1)
    {
        FPGA_Interface[Putout_B_PluginUnit_11 - 2].SHUTDOWN = Shutdown_2;
    }
    else if (OpticalNum == 2)
    {
        FPGA_Interface[Putout_C_PluginUnit_15 - 2].SHUTDOWN = Shutdown_3;
    }
    else if (OpticalNum == 3)
    {
        FPGA_Interface[Putout_A_PluginUnit_7 - 2].SHUTDOWN = Shutdown_1;
        FPGA_Interface[Putout_B_PluginUnit_11 - 2].SHUTDOWN = Shutdown_2;
        FPGA_Interface[Putout_C_PluginUnit_15 - 2].SHUTDOWN = Shutdown_3;
    }
    else if (OpticalNum == 4)
    {
#ifdef LVAC_online
        //		FPGA_Interface[Putout_A_PluginUnit_7-2].SHUTDOWN=Shutdown_1;

        FPGA_Interface[InvPWM_A_PluginUnit_9 - 2].INV_SHUTDOWN = Shutdown_2;
        FPGA_Interface[InvPWM_B_PluginUnit_10 - 2].INV_SHUTDOWN = Shutdown_3;
        FPGA_Interface[InvPWM_C_PluginUnit_11 - 2].INV_SHUTDOWN = Shutdown_4;
        FPGA_Interface[InvPWM_N_PluginUnit_12 - 2].INV_SHUTDOWN = Shutdown_5;
#endif
    }
    else
    {
    }
}

#ifdef HVAC_online
static uchar s_module_fo_phases; /* 0：701尚未布防，原FO/FB配置保留。 */

/* 同相第一张501占位0~7，第二张占位8~15。保留各组的实际插口位置。 */
static ushort PhaseModuleMask(ushort first_group, ushort second_group)
{
    return (ushort)(first_group | (second_group << 8U));
}

void Port_ApplyModuleShutdownMask(void)
{
    Port_ModuleShutdown(3U,
                        (ushort)~PhaseModuleMask(HA_PM_A1, HA_PM_A2),
                        (ushort)~PhaseModuleMask(HA_PM_B1, HA_PM_B2),
                        (ushort)~PhaseModuleMask(HA_PM_C1, HA_PM_C2), 0U, 0U);
}
#endif /* HVAC_online */

ushort Port_EffectiveFOEnable(ushort input_plugin)
{
    switch (input_plugin)
    {
        case Input_A_PluginUnit_8:
#ifdef HVAC_online
            return (s_module_fo_phases & 1U) != 0U
                       ? Port_FO_1_EN & PhaseModuleMask(HA_PM_A1, HA_PM_A2) : 0U;
#else
            return Port_FO_1_EN;
#endif
        case Input_B_PluginUnit_12:
#ifdef HVAC_online
            return (s_module_fo_phases & 2U) != 0U
                       ? Port_FO_2_EN & PhaseModuleMask(HA_PM_B1, HA_PM_B2) : 0U;
#else
            return Port_FO_2_EN;
#endif
        case Input_C_PluginUnit_16:
#ifdef HVAC_online
            return (s_module_fo_phases & 4U) != 0U
                       ? Port_FO_3_EN & PhaseModuleMask(HA_PM_C1, HA_PM_C2) : 0U;
#else
            return Port_FO_3_EN;
#endif
        default:
            return 0U;
    }
}

ushort Port_EffectiveFBEnable(void)
{
    ushort enabled = Port_FB_EN;

#ifdef HVAC_online
    /* 仅裁剪三块701的汇总FB，保留主控其余插件的独立配置。 */
    if (Port_EffectiveFOEnable(Input_A_PluginUnit_8) == 0U)
        enabled &= ~(1U << (Input_A_PluginUnit_8 - 2U));
    if (Port_EffectiveFOEnable(Input_B_PluginUnit_12) == 0U)
        enabled &= ~(1U << (Input_B_PluginUnit_12 - 2U));
    if (Port_EffectiveFOEnable(Input_C_PluginUnit_16) == 0U)
        enabled &= ~(1U << (Input_C_PluginUnit_16 - 2U));
#endif
    return enabled;
}

#ifdef HVAC_online
uchar Port_SetModuleFOArmed(uchar phase_mask)
{
    if (phase_mask != s_module_fo_phases)
    {
        s_module_fo_phases = phase_mask;
        if (phase_mask == 0U)
        {
            Port_FB_EN_Set();
            Port_FO_EN_Set(3U, 0U, 0U, 0U, 0U);
        }
        else
        {
            Port_FO_EN_Set(3U, 0U, 0U, 0U, 0U);
            Port_FB_EN_Set();
        }
        return 1U;
    }
    return 0U;
}

ushort Port_UnitLinkRead(ushort plugin)
{
    return FPGA_Interface[plugin - 2U].UNIT_LINK;
}

ushort Port_UnitLinkLossRead(ushort plugin)
{
    return FPGA_Interface[plugin - 2U].UNIT_LINK_LOSS;
}

void Port_UnitLinkLossClear(ushort plugin, ushort channels)
{
    FPGA_Interface[plugin - 2U].UNIT_LINK_LOSS = channels;
}

ushort Port_UnitFanConfigSupported(ushort plugin)
{
    return FPGA_Interface[plugin - 2U].FAN_CONFIG_ID == 0x4641U;
}

void Port_UnitFanConfigure(ushort plugin, ushort off_count, ushort on_count, ushort generation)
{
    volatile FPGA_InterfaceBLOCK *board = &FPGA_Interface[plugin - 2U];

    board->FAN_CONFIG_OFF = off_count;
    board->FAN_CONFIG_ON = on_count;
    board->FAN_CONFIG_COMMIT = 0x8000U | generation;
}

void Port_UnitFanConfigRead(ushort plugin, ushort generation, ushort *confirmed, ushort *rejected)
{
    volatile FPGA_InterfaceBLOCK *board = &FPGA_Interface[plugin - 2U];
    ushort ack = board->FAN_CONFIG_ACK;
    ushort reject = board->FAN_CONFIG_REJECT;

    *confirmed = ((ack >> 8U) == generation) ? (ack & 0xFFU) : 0U;
    *rejected = ((reject >> 8U) == generation) ? (reject & 0xFFU) : 0U;
}
#endif /* HVAC_online */

/*==================================================================
 * 函数功能: 设置光纤故障输入的各相使能位图
 * 输入参数: OpticalNum：光纤插件/组编号
 *           INV_A_FOEN：A 相逆变故障光纤使能位图
 *           INV_B_FOEN：B 相逆变故障光纤使能位图
 *           INV_C_FOEN：C 相逆变故障光纤使能位图
 *           INV_N_FOEN：中性桥故障光纤使能位图
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Port_FO_EN_Set(ushort OpticalNum, ushort INV_A_FOEN, ushort INV_B_FOEN, ushort INV_C_FOEN, ushort INV_N_FOEN)
{
    FPGA_Interface[Input_A_PluginUnit_8 - 2].FO_EN = Port_EffectiveFOEnable(Input_A_PluginUnit_8);
    if (OpticalNum == 3U)
    {
        FPGA_Interface[Input_B_PluginUnit_12 - 2].FO_EN = Port_EffectiveFOEnable(Input_B_PluginUnit_12);
        FPGA_Interface[Input_C_PluginUnit_16 - 2].FO_EN = Port_EffectiveFOEnable(Input_C_PluginUnit_16);
    }
    else if (OpticalNum == 4)
    {
#ifdef LVAC_online
        FPGA_Interface[InvPWM_A_PluginUnit_9 - 2].INV_FO_EN = INV_A_FOEN;
        FPGA_Interface[InvPWM_B_PluginUnit_10 - 2].INV_FO_EN = INV_B_FOEN;
        FPGA_Interface[InvPWM_C_PluginUnit_11 - 2].INV_FO_EN = INV_C_FOEN;
        FPGA_Interface[InvPWM_N_PluginUnit_12 - 2].INV_FO_EN = INV_N_FOEN;
#endif
    }
    else
    {
    }
}

/*==================================================================
 * 函数功能: 设置独立逆变桥的 FPGA 死区计数
 * 输入参数: Deadzone_inv：逆变桥死区 FPGA 计数
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void INV_CONTROL_DEADZONE_Set(ushort Deadzone_inv)
{
#ifdef LVAC_online
    FPGA_Interface[InvPWM_A_PluginUnit_9 - 2].INV_CTL.Bbits.DEADZONE = Deadzone_inv;
    FPGA_Interface[InvPWM_B_PluginUnit_10 - 2].INV_CTL.Bbits.DEADZONE = Deadzone_inv;
    FPGA_Interface[InvPWM_C_PluginUnit_11 - 2].INV_CTL.Bbits.DEADZONE = Deadzone_inv;
    FPGA_Interface[InvPWM_N_PluginUnit_12 - 2].INV_CTL.Bbits.DEADZONE = Deadzone_inv;
#endif
}

/*==================================================================
 * 函数功能: 设置独立逆变桥的 FPGA 周期计数
 * 输入参数: Period_inv：逆变桥周期 FPGA 计数
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void INV_CONTROL_PERIOD_Set(ushort Period_inv)
{
#ifdef LVAC_online
    FPGA_Interface[InvPWM_A_PluginUnit_9 - 2].INV_CTL.Bbits.TAD_DIV = Period_inv;
    FPGA_Interface[InvPWM_B_PluginUnit_10 - 2].INV_CTL.Bbits.TAD_DIV = Period_inv;
    FPGA_Interface[InvPWM_C_PluginUnit_11 - 2].INV_CTL.Bbits.TAD_DIV = Period_inv;
    FPGA_Interface[InvPWM_N_PluginUnit_12 - 2].INV_CTL.Bbits.TAD_DIV = Period_inv;
#endif
}

/*==================================================================
 * 函数功能: 写入 FPGA BKU 故障锁存控制值
 * 输入参数: LatchValue：BKU 故障锁存控制值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Port_BKU_Send(uchar LatchValue)
{
    if (LatchValue)
    {
        //		FPGA_Interface[MainControl_PluginUnit_4-2].BKU=0;
        //		Delay(1000);
        FPGA_Interface[MainControl_PluginUnit_4 - 2].BKU = 0XFFFF;
    }
    else
    {
        //		FPGA_Interface[MainControl_PluginUnit_4-2].BKU=0XFFFF;
        //		Delay(1000);
        FPGA_Interface[MainControl_PluginUnit_4 - 2].BKU = 0;
    }
}

/*==================================================================
 * 函数功能: 写入端口 FB 硬件保护使能位图
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Port_FB_EN_Set(void)
{
    FPGA_Interface[MainControl_PluginUnit_4 - 2].FB_EN = Port_EffectiveFBEnable();
}

/*==================================================================
 * 函数功能: 设置端口 FPGA 的硬件故障复位控制位
 * 输入参数: RST_FB：端口硬件故障复位控制值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Port_DSPControl_FBRST(uchar RST_FB)
{
    if (RST_FB)
    {
        Port_DSP_CTL.Bbits.HW_RST = 1;
    }
    else
    {
        Port_DSP_CTL.Bbits.HW_RST = 0;
    }
    FPGA_Interface[MainControl_PluginUnit_4 - 2].DSP_CTL.DSP_CTL16 = Port_DSP_CTL.DSP_CTL16;
}

/*==================================================================
 * 函数功能: 设置端口 FPGA 的全模块关断控制位
 * 输入参数: AllShutdown：全模块关断控制值
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void Port_DSPControl_AllShutdown(uchar AllShutdown)
{
    if (AllShutdown)
    {
        Port_DSP_CTL.Bbits.SHUTDOWN = 1;
    }
    else
    {
        Port_DSP_CTL.Bbits.SHUTDOWN = 0;
    }
    FPGA_Interface[MainControl_PluginUnit_4 - 2].DSP_CTL.DSP_CTL16 = Port_DSP_CTL.DSP_CTL16;
}

/*==================================================================
 * 函数功能: 读取指定插件、模块的实时或锁存状态字
 * 输入参数: CJ_index：插件编号，按原工程从 2 开始，寄存器数组下标为编号减 2
 *           MK_index：插件内模块编号，从 1 开始，模块数组下标为编号减 1
 *           latch：0 读取实时寄存器，非零读取锁存寄存器
 * 输出参数: 无指针输出；读取原 FPGA 实时或锁存寄存器，结果通过返回值提供。
 * 返回值: 模块的 16 位原始状态字
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
ushort Module_StatusRead(uchar CJ_index, uchar MK_index, uchar latch)
{
    ushort ModuleStatus_Temp; /* ModuleStatus_Temp：读取的模块/插件原始状态字。 */
    if (latch == 0)
    {
        ModuleStatus_Temp = FPGA_Interface[CJ_index - 2].STATUS[MK_index].STATUS16;
    }
    else
    {
        //		ModuleStatus_Temp=FPGA_Interface[CJ_index-2].STATUS_LATCH[MK_index].STATUS16;
    }

    return ModuleStatus_Temp;
}

/*==================================================================
 * 函数功能: 读取指定插件所有模块的汇总状态
 * 输入参数: CJ_index：插件编号，按原工程从 2 开始，寄存器数组下标为编号减 2
 *           latch：0 读取实时寄存器，非零读取锁存寄存器
 * 输出参数: 无指针输出；读取原 FPGA 实时或锁存寄存器，结果通过返回值提供。
 * 返回值: 插件的 16 位原始汇总状态
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
ushort Module_TotalStatusRead(uchar CJ_index, uchar latch)
{
    ushort ModuleStatus_Temp; /* ModuleStatus_Temp：读取的模块/插件原始状态字。 */
    if (latch == 0)
    {
        ModuleStatus_Temp = FPGA_Interface[CJ_index - 2].OR_STATUS.STATUS16;
    }
    else
    {
        ModuleStatus_Temp = FPGA_Interface[CJ_index - 2].OR_STATUS.STATUS16;
    }

    return ModuleStatus_Temp;
}

/*==================================================================
 * 函数功能: 读取S301的8路开入原始字
 * 输入参数: latch：0读实时DIN，1读故障时锁存DIN_LATCH
 * 输出参数: 无；不改写运行命令或其他开关状态
 * 返回值: bit0~7依次为急停、复归、远方、开机、停机、KT1合位、QF1合位、QF1分位
 * 执行说明: 301 FPGA已完成输入反相及滤波，寄存器1表示触点闭合。
 *           实时DSP字节地址0x600003C0，锁存地址0x600003C2。
 *================================================================*/
ushort Port_DINRead(uchar latch)
{
    if (latch == 0U)
    {
        return FPGA_Interface[IO_PluginUnit_2 - 2].DIN.SwitchINOUT16;
    }
    return FPGA_Interface[IO_PluginUnit_2 - 2].DIN_LATCH.SwitchINOUT16;
}

/*==================================================================
 * 函数功能: 读取端口 AD 硬件保护的实时或锁存值
 * 输入参数: latch：0 读取实时寄存器，非零读取锁存寄存器
 * 输出参数: 无指针输出；读取原 FPGA 实时或锁存寄存器，结果通过返回值提供。
 * 返回值: 16 位 AD 保护位图
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
ushort Port_AD_ProtectStatusRead(uchar latch)
{
    ushort AD_ProtectStatus_Temp; /* AD_ProtectStatus_Temp：读取并选择后的端口 AD 保护原始状态字。 */
    if (latch == 0)
    {
        AD_ProtectStatus_Temp = FPGA_Interface[Analog_PluginUnit_3 - 2].AD_PROT;
    }
    else
    {
        AD_ProtectStatus_Temp = FPGA_Interface[Analog_PluginUnit_3 - 2].AD_PROT_LATCH;
    }

    return AD_ProtectStatus_Temp;
}

/*==================================================================
 * 函数功能: 读取指定插件的光纤故障实时或锁存值
 * 输入参数: CJ_index：插件编号，按原工程从 2 开始，寄存器数组下标为编号减 2
 *           latch：0 读取实时寄存器，非零读取锁存寄存器
 * 输出参数: 无指针输出；读取原 FPGA 实时或锁存寄存器，结果通过返回值提供。
 * 返回值: 16 位光纤故障位图
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
ushort Port_FOStatusRead(uchar CJ_index, uchar latch)
{
    ushort FOStatus_Temp; /* FOStatus_Temp：读取的插件 FO 原始故障位图。 */
    if (latch == 0)
    {
        FOStatus_Temp = FPGA_Interface[CJ_index - 2].FO;
    }
    else
    {
        FOStatus_Temp = FPGA_Interface[CJ_index - 2].FO_LATCH;
    }

    return FOStatus_Temp;
}

/*==================================================================
 * 函数功能: 读取端口 FB 故障的实时或锁存值
 * 输入参数: latch：0 读取实时寄存器，非零读取锁存寄存器
 * 输出参数: 无指针输出；读取原 FPGA 实时或锁存寄存器，结果通过返回值提供。
 * 返回值: 16 位 FB 故障位图
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
ushort Port_FBStatusRead(uchar latch)
{
    ushort FBStatus_Temp; /* FBStatus_Temp：读取的端口 FB 原始故障位图。 */
    if (latch == 0)
    {
        FBStatus_Temp = FPGA_Interface[MainControl_PluginUnit_4 - 2].FB;
    }
    else
    {
        FBStatus_Temp = FPGA_Interface[MainControl_PluginUnit_4 - 2].FB_LATCH;
    }

    return FBStatus_Temp;
}

/*==================================================================
 * 函数功能: 读取独立逆变桥光纤故障的实时或锁存值
 * 输入参数: CJ_index：插件编号，按原工程从 2 开始，寄存器数组下标为编号减 2
 *           latch：0 读取实时寄存器，非零读取锁存寄存器
 * 输出参数: 无指针输出；读取原 FPGA 实时或锁存寄存器，结果通过返回值提供。
 * 返回值: 16 位逆变桥光纤故障位图
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
ushort Port_INVFOStatusRead(uchar CJ_index, uchar latch)
{
    ushort INVFOStatus_Temp; /* INVFOStatus_Temp：读取的独立逆变桥 FO 原始故障位图。 */
    if (latch == 0)
    {
        INVFOStatus_Temp = FPGA_Interface[CJ_index - 2].INV_FO;
    }
    else
    {
        INVFOStatus_Temp = FPGA_Interface[CJ_index - 2].INV_FO_LATCH;
    }

    return INVFOStatus_Temp;
}
