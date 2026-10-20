#ifndef SST_FAULT_H_
#define SST_FAULT_H_

/* Reporting only. Numeric levels match F28377S_Plat/ErrorMacro.h exactly. */
/* SST_FAULT_LEVEL_1：参考一级故障等级编码 0。 */
#define SST_FAULT_LEVEL_1 0U
/* SST_FAULT_LEVEL_2：参考二级故障等级编码 1。 */
#define SST_FAULT_LEVEL_2 1U
/* SST_FAULT_LEVEL_3：参考三级故障等级编码 2。 */
#define SST_FAULT_LEVEL_3 2U
/* SST_FAULT_LEVEL_4：参考四级故障等级编码 3。 */
#define SST_FAULT_LEVEL_4 3U
/* SST_FAULT_LEVEL_NONE：参考无故障等级编码 4。 */
#define SST_FAULT_LEVEL_NONE 4U

/* SST_FAULT_CURRENT_CAPACITY：当前故障表容量 32。 */
#define SST_FAULT_CURRENT_CAPACITY 32U
/* SST_FAULT_HISTORY_CAPACITY：详细 RAM 历史容量 32。 */
#define SST_FAULT_HISTORY_CAPACITY 32U
/* SST_FAULT_LEGACY_HISTORY_WORDS：参考组 19 的历史字数 34，字 0 为写位置。 */
#define SST_FAULT_LEGACY_HISTORY_WORDS 34U

/* ABCDE: A=layer, BC=module, DE=condition. Module 91 is SST-specific. */
/* SST_FAULT_NONE：无上报故障码 0。 */
#define SST_FAULT_NONE 0U
/* SST_FAULT_HW_UNCLASSIFIED：未能细分的硬件故障五位上报码。 */
#define SST_FAULT_HW_UNCLASSIFIED 19101U
/* SST_FAULT_HW_PORT_OC：端口硬件过流五位上报码。 */
#define SST_FAULT_HW_PORT_OC 19102U
/* SST_FAULT_HW_PORT_OV：端口硬件过压五位上报码。 */
#define SST_FAULT_HW_PORT_OV 19103U
/* SST_FAULT_HW_MODULE：模块汇总故障五位上报码。 */
#define SST_FAULT_HW_MODULE 19104U
/* SST_FAULT_HW_MODULE_OC：模块过流五位上报码。 */
#define SST_FAULT_HW_MODULE_OC 19105U
/* SST_FAULT_HW_MODULE_OV：模块过压五位上报码。 */
#define SST_FAULT_HW_MODULE_OV 19106U
/* SST_FAULT_HW_MODULE_OT：模块过温五位上报码。 */
#define SST_FAULT_HW_MODULE_OT 19107U
/* SST_FAULT_HW_CHB_BRIDGE：模块工频 CHB 桥故障五位上报码。 */
#define SST_FAULT_HW_CHB_BRIDGE 19108U
/* SST_FAULT_HW_DAB_BRIDGE：模块 DAB 桥故障五位上报码。 */
#define SST_FAULT_HW_DAB_BRIDGE 19109U
/* SST_FAULT_HW_MODULE_SINGLE：单模块故障五位上报码。 */
#define SST_FAULT_HW_MODULE_SINGLE 19110U
/* SST_FAULT_HW_MODULE_UV：模块欠压五位上报码。 */
#define SST_FAULT_HW_MODULE_UV 19111U
/* SST_FAULT_HW_MODULE_OV2：模块二级过压五位上报码。 */
#define SST_FAULT_HW_MODULE_OV2 19112U
/* SST_FAULT_HW_FAN：风扇故障五位上报码。 */
#define SST_FAULT_HW_FAN 19113U
/* SST_FAULT_HW_CONTROLLER：控制器故障五位上报码。 */
#define SST_FAULT_HW_CONTROLLER 19114U
/* SST_FAULT_HW_AUX_SUPPLY：辅助电源故障五位上报码。 */
#define SST_FAULT_HW_AUX_SUPPLY 19115U
/* SST_FAULT_HW_EPO：紧急关断输入故障五位上报码。 */
#define SST_FAULT_HW_EPO 19116U
/* SST_FAULT_PRECHARGE_TIMEOUT：预充超时五位上报码，沿用参考 20103。 */
#define SST_FAULT_PRECHARGE_TIMEOUT 20103U
/* SST_FAULT_PLL_TIMEOUT：PLL 锁定超时五位上报码。 */
#define SST_FAULT_PLL_TIMEOUT 29102U
/* SST_FAULT_SW_AC_OC：软件交流过流五位上报码。 */
#define SST_FAULT_SW_AC_OC 29103U
/* SST_FAULT_SW_DC_OV：软件直流过压五位上报码。 */
#define SST_FAULT_SW_DC_OV 29104U
/* SST_FAULT_BAD_CONFIGURATION：控制配置非法五位上报码。 */
#define SST_FAULT_BAD_CONFIGURATION 29105U
/* SST_FAULT_CONTROL_RUNTIME：运行时采样或控制计算失效五位上报码。 */
#define SST_FAULT_CONTROL_RUNTIME 29109U
/* SST_FAULT_PHASE_SEQUENCE：相序异常五位上报码。 */
#define SST_FAULT_PHASE_SEQUENCE 29106U
/* SST_FAULT_FREQUENCY：频率异常五位上报码。 */
#define SST_FAULT_FREQUENCY 29107U
/* SST_FAULT_SWITCH_FEEDBACK：进线/旁路反馈不符，五位上报码；不占用已有相序及频率故障码。 */
#define SST_FAULT_SWITCH_FEEDBACK 29108U
/* SST_FAULT_CONTACTOR：接触器故障五位上报码。 */
#define SST_FAULT_CONTACTOR 39101U
/* SST_FAULT_BREAKER：断路器故障五位上报码。 */
#define SST_FAULT_BREAKER 39102U
/* SST_FAULT_COMMUNICATION：通信故障五位上报码。 */
#define SST_FAULT_COMMUNICATION 39103U
/* SST_FAULT_SYNC：同步故障五位上报码。 */
#define SST_FAULT_SYNC 39104U

typedef struct
{
    unsigned short code; /* code：参考五位 SST_FAULT_* 上报故障码，0 表示无故障。 */
    unsigned short level; /* level：参考故障等级，0 最严重、4 无故障。 */
    unsigned short control_fault; /* control_fault：原 CHB_FAULT_* 控制故障编号。 */
    unsigned short port_fault; /* port_fault：旧 Port_RealFault 的原始 16 位故障字。 */
    unsigned short fault_type; /* fault_type：旧 FaultTypeFLAG 原始 16 位故障分类位图。 */
    unsigned short port_latch; /* port_latch：端口 AD 保护锁存的后台副本。 */
    unsigned short fb_latch; /* fb_latch：端口 FB 硬件故障锁存的后台副本。 */
    unsigned short fo_a; /* fo_a：A 相光纤故障锁存的后台副本。 */
    unsigned short fo_b; /* fo_b：B 相光纤故障锁存的后台副本。 */
    unsigned short fo_c; /* fo_c：C 相光纤故障锁存的后台副本。 */
    unsigned short module_dispose; /* module_dispose：原模块故障处置计数的后台副本。 */
    unsigned long tick_10ms;
    /* tick_10ms：10 ms 软件节拍；故障记录中为后台观察时刻，不是精确跳闸时间。 */ /* Background observation time, not trip time. */
    unsigned long sequence; /* sequence：维护协议请求序号，或详细故障记录的递增事件序号。 */
} SST_FAULT_RECORD;

#ifdef __cplusplus
extern "C"
{
#endif

    /* Init and Service run only in the background, never from an ISR. */
    /*==================================================================
 * 函数功能: 清空当前故障与 RAM 历史，初始化参考历史环写位置
 * 输入参数: 无
 * 输出参数: 当前故障表、详细 RAM 历史及参考组 19 历史环。
 * 返回值: 无
 * 执行说明: 纯后台报告服务；不重新读取 FPGA，不关中断，不改变原保护或 PWM 决策。
 *================================================================*/
    void SstFault_Init(void);
    /*==================================================================
 * 函数功能: 把原控制/硬件故障映射为参考五位故障码和等级并更新报告
 * 输入参数: 无
 * 输出参数: 当前故障表、详细 RAM 历史及参考组 19 历史环。
 * 返回值: 无
 * 执行说明: 纯后台报告服务；不重新读取 FPGA，不关中断，不改变原保护或 PWM 决策。
 *================================================================*/
    void SstFault_Service(void);
    /*==================================================================
 * 函数功能: 按当前故障索引读取报告码
 * 输入参数: index：当前故障表索引，范围 0..当前故障数-1
 * 输出参数: 无。
 * 返回值: 故障码；越界返回 0
 * 执行说明: 纯后台报告服务；不重新读取 FPGA，不关中断，不改变原保护或 PWM 决策。
 *================================================================*/
    unsigned short SstFault_GetCode(unsigned short index);
    /*==================================================================
 * 函数功能: 读取当前首个故障报告码
 * 输入参数: 无
 * 输出参数: 无。
 * 返回值: 首故障码；无故障返回 0
 * 执行说明: 纯后台报告服务；不重新读取 FPGA，不关中断，不改变原保护或 PWM 决策。
 *================================================================*/
    unsigned short SstFault_GetFirstCode(void);
    /*==================================================================
 * 函数功能: 读取当前故障中的最高严重等级
 * 输入参数: 无
 * 输出参数: 无。
 * 返回值: 参考等级 0..4，4 表示无故障
 * 执行说明: 纯后台报告服务；不重新读取 FPGA，不关中断，不改变原保护或 PWM 决策。
 *================================================================*/
    unsigned short SstFault_GetLevel(void);
    /*==================================================================
 * 函数功能: 读取当前故障去重数量
 * 输入参数: 无
 * 输出参数: 无。
 * 返回值: 当前故障数量，最多 32
 * 执行说明: 纯后台报告服务；不重新读取 FPGA，不关中断，不改变原保护或 PWM 决策。
 *================================================================*/
    unsigned short SstFault_GetActiveCount(void);
    /*==================================================================
 * 函数功能: 读取 RAM 详细历史的有效记录数
 * 输入参数: 无
 * 输出参数: 无。
 * 返回值: 有效历史数量，最多 32
 * 执行说明: 纯后台报告服务；不重新读取 FPGA，不关中断，不改变原保护或 PWM 决策。
 *================================================================*/
    unsigned short SstFault_GetHistoryCount(void);
    /*==================================================================
 * 函数功能: 按新到旧读取详细历史故障码
 * 输入参数: newest_index：历史记录从新到旧的索引，0 为最新
 * 输出参数: 无。
 * 返回值: 故障码；越界返回 0
 * 执行说明: 纯后台报告服务；不重新读取 FPGA，不关中断，不改变原保护或 PWM 决策。
 *================================================================*/
    unsigned short SstFault_GetHistoryCode(unsigned short newest_index);
    /*==================================================================
 * 函数功能: 读取 RAM 历史中最高严重等级
 * 输入参数: 无
 * 输出参数: 无。
 * 返回值: 参考等级 0..4
 * 执行说明: 纯后台报告服务；不重新读取 FPGA，不关中断，不改变原保护或 PWM 决策。
 *================================================================*/
    unsigned short SstFault_GetHistoryLevel(void);
    /* Reference FuncCode group 19: word0=next slot, slots1..33=code ring.
 * A zero session separator precedes the first code after Init. RAM only. */
    /*==================================================================
 * 函数功能: 读取参考 FuncCode 组 19 的原布局历史环字
 * 输入参数: index：参考组 19 历史环字索引，范围 0..33
 * 输出参数: 无。
 * 返回值: 字 0 为下次写位置，字 1..33 为故障码；越界返回 0
 * 执行说明: 纯后台报告服务；不重新读取 FPGA，不关中断，不改变原保护或 PWM 决策。
 *================================================================*/
    unsigned short SstFault_GetLegacyHistoryWord(unsigned short index);
    /* Returns 1 on success, 0 for NULL/out-of-range. Newest record has index 0. */
    /*==================================================================
 * 函数功能: 按新到旧复制一条详细历史记录给调用者
 * 输入参数: newest_index：历史记录从新到旧的索引，0 为最新
 *           record：输出详细故障记录的目标指针，不可为空
 * 输出参数: record 指向的一条历史记录，newest_index=0 读取最新。
 * 返回值: 1 表示复制成功，0 表示指针为空或越界
 * 执行说明: 后台读取 RAM 历史，不访问硬件、不关中断；输出为后台观察记录。
 *================================================================*/
    unsigned short SstFault_GetHistory(unsigned short newest_index, SST_FAULT_RECORD *record);

#ifdef __cplusplus
}
#endif
#endif
