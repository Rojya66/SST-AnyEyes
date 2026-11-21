//###########################################################################
// FILE: SstHardwareParameters.h
// FUNCTION: EEPROM中的光纤接线、保护使能及端口管理参数；不包含任何启停命令。
//###########################################################################
#ifndef SST_HARDWARE_PARAMETERS_H
#define SST_HARDWARE_PARAMETERS_H
#include "SstParamTypes.h"

/* v3镜像中第50字起的固定顺序；与屏幕57项的索引不同，禁止混用。 */
enum
{
    SST_HW_SYSTEM_MODE = 0, /* bit0运行/测试，bit1自动/手动；不是run_enable。 */
    SST_HW_AD_ENABLE,      /* 202的AD_PROT_EN。 */
    SST_HW_FO_A_ENABLE,    /* A相701的OP_IN使能。 */
    SST_HW_FO_B_ENABLE,    /* B相701的故障光纤使能。 */
    SST_HW_FO_C_ENABLE,    /* C相701的故障光纤使能。 */
    SST_HW_FB_ENABLE,      /* 主控汇总FB使能，bit0/6/10/14等。 */
    SST_HW_PM_A1,          /* 六张501的8位TX/RX通道选择，顺序A1/A2/B1/B2/C1/C2。 */
    SST_HW_PM_A2,          /* A相第二张501的有效光纤通道位图。 */
    SST_HW_PM_B1,          /* B相第一张501的有效光纤通道位图。 */
    SST_HW_PM_B2,          /* B相第二张501的有效光纤通道位图。 */
    SST_HW_PM_C1,          /* C相第一张501的有效光纤通道位图。 */
    SST_HW_PM_C2,          /* C相第二张501的有效光纤通道位图。 */
    SST_HW_PORT_ON_STATE,  /* 旧四端口联动存在位图，不是开机命令。 */
    SST_HW_MAIN_MODE,      /* 原端口主工作方式0..3。 */
    SST_HW_STATCOM_MODE,   /* 原STATCOM子方式0..3。 */
    SST_HW_APF_MODE,       /* 原APF子方式0..2。 */
    SST_HARDWARE_PARAMETER_WORDS /* v3硬件配置段固定为16个16位字。 */
};

#ifdef __cplusplus
extern "C"
{
#endif
    /*==================================================================
 * 函数功能: 读取16个实际活动硬件参数，组成v3镜像最后一段
 * 输入参数: words：至少16字缓存，按本头文件枚举顺序填写
 * 返回值: 无
 * 执行说明: 初始化或主循环调用，允许实时中断抢占，不关全局中断。
 *================================================================*/
    void SstHardwareParameters_Read(uint16_t *words);
    /*==================================================================
 * 函数功能: 校验16字硬件配置的有效位宽和方式范围，不修改活动对象
 * 输入参数: words：包含全部16个硬件字的待校验缓存
 * 返回值: SST_PARAM_OK有效；SST_PARAM_RANGE字段非法
 * 执行说明: 初始化或主循环调用，允许实时中断抢占，不关全局中断。
 *================================================================*/
    uint16_t SstHardwareParameters_Validate(const uint16_t *words);
    /*==================================================================
 * 函数功能: 将已整幅校验的硬件参数写入DSP变量及对应FPGA寄存器
 * 输入参数: words：已通过完整镜像校验的16字硬件参数缓存
 * 返回值: 无
 * 执行说明: 初始化或主循环调用，允许实时中断抢占，不关全局中断。
 *================================================================*/
    void SstHardwareParameters_Apply(const uint16_t *words);
#ifdef __cplusplus
}
#endif
#endif
