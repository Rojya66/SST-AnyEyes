//###########################################################################
// FILE: SstDpramCommand.c
// FUNCTION: V1.1旧块1按有效掩码解析、停机优先、受控开关维护；PWM只由核心决定。
//###########################################################################
#include "SstDpramProtocol.h"
#include "SstCommand.h"
#include "SstParams.h"
#include "ChbDabControl.h"
#include "ChbBuildConfig.h"
#include "PortStatusData.h"

/* 同一字段的协议范围与本机解析范围；未接入位只记录，不拒绝整包。 */
typedef struct
{
    uint16_t protocol_mask; /* Word文档有效位，用于区分保留内容与未接入的有效字段。 */
    uint16_t local_mask; /* 本机提取位，只允许明确绑定的本地命令参与执行及联锁。 */
} SST_DPRAM_CONTROL_MASK;

/* 每行依次为{协议有效位，本机提取位}，只在下面的一次接收循环中使用。
 * 未接入的其他端口/模式/外部开关位不会转成本机动作，也不会阻断同包HVAC START。
 * 传感器/电网字段兼容接收描述信息，不据此动态改变采样系数或保护。
 * 不修改原始RAM缓冲及共享控制块，便于对照屏幕原值并避免与ARM后续写入竞争。 */
static const SST_DPRAM_CONTROL_MASK s_control_masks[16] =
{
    {0x0000U, 0x0000U}, /* word0  / 00F0：保留字忽略。 */
    {0x0000U, 0x0000U}, /* word1  / 00F1：保留字忽略。 */
    {0x000FU, 0x0001U}, /* word2  / 00F2：只提取HVAC启动bit0。 */
    {0x000FU, 0x0001U}, /* word3  / 00F3：只提取HVAC停止bit0。 */
    {0x3FFFU, 0x0001U}, /* word4  / 00F4：恒压能力确认；未接入模式不改变实际ac_mode。 */
    {0x0000U, 0x0000U}, /* word5  / 00F5：保留字忽略。 */
    {0x000FU, 0x0003U}, /* word6  / 00F6：bit0合KT1、bit1合QF1；bit2/3忽略。 */
    {0x000FU, 0x0003U}, /* word7  / 00F7：bit0释放KT1、bit1分QF1；bit2/3忽略。 */
    {0x0003U, 0x0003U}, /* word8  / 00F8：DAB、CHB解脉冲。 */
    {0x0003U, 0x0003U}, /* word9  / 00F9：DAB、CHB封脉冲。 */
    {0x0001U, 0x0001U}, /* word10 / 00FA：故障复位。 */
    {0x0003U, 0x0003U}, /* word11 / 00FB：合闸许可及禁止，两位仍互斥。 */
    {0x000FU, 0x0001U}, /* word12 / 00FC：HVAC描述；不靠其他端口位触发本机动作。 */
    {0x00FFU, 0x00FFU}, /* word13 / 00FD：PT/CT描述，不修改采样比例。 */
    {0x003FU, 0x003FU}, /* word14 / 00FE：电网属性描述，不修改PLL或保护。 */
    {0x0000U, 0x0000U}  /* word15 / 00FF：保留字忽略。 */
};

/*==================================================================
 * 函数功能: 解释旧16字控制块，把合法命令送入现有控制核心
 * 输入参数: words：完整块1；仅解析协议有效位，保留位/保留字忽略
 * 输出参数: 启停请求、复位请求、许可或受控开关请求；重复START不重置启动
 * 返回值: 受理结果；表示请求通过检查，实际运行仍须读状态机及故障
 * 执行说明: 仅后台；未接入位忽略并记录，不拒绝正常本机命令；STOP/OFF优先。
 * 调用关系: Host读邮箱1并复制块1 -> 本函数提取/校验 -> SstControlCommand_ApplyRequests赋值。
 *           本函数不直接置1任何使能，也不写PWM；成功只代表请求已提交。
 *================================================================*/
uint16_t SstDpramCommand_ProcessControlBlock(const uint16_t *words)
{
    uint16_t index; /* 控制块校验字序号。 */
    uint16_t permission; /* 本笔处理后的外部合闸许可。 */
    uint16_t command_words[16]; /* 已按协议掩码提取的本笔命令，后续只检查此副本。 */
#if (SST_ENABLE_DPRAM == 0U) || (SST_ALLOW_REMOTE_COMMANDS == 0U)
    (void)words;
    return SST_PARAM_DENIED;
#endif
    g_sst_dpram_screen_result.error_word = 0xFFFFU;
    g_sst_dpram_screen_result.ignored_control_words = 0U;
    /* 第1步：words[0..15]是PORT_Control_DATA_Buf.DPRAM_Block1Buf的原始字。
     * 下标对应Modbus 00F0+index，DSP地址660001E0+2*index。
     * 每个字直接按本机提取位生成command_words，后面没有“未接入位非零就退整包”的检查。
     * 例：word2=49409(C101)，只取bit0得1，即HVAC_Start；模式word4=2也不会挡住它。
     * protocol_mask只用于诊断：有协议有效位未接入时，置ignored_control_words对应字号bit。
     * 纯保留内容不记录为未接入功能；原始words保持原值供CCS核对。 */
    for (index = 0U; index < 16U; ++index)
    {
        command_words[index] = words[index] & s_control_masks[index].local_mask;
        if ((words[index] & s_control_masks[index].protocol_mask &
             (uint16_t)~s_control_masks[index].local_mask) != 0U)
        {
            /* 仅诊断，不return、不把其他端口/未接入功能误当成HVAC动作。 */
            g_sst_dpram_screen_result.ignored_control_words |= (uint16_t)(1U << index);
        }
    }

    /* 第2步：word3 / 00F3 bit0整机STOP优先，EmergencyStop内把两使能清0。
     * 再请求断开维护开关；这里即返回，不再受理同包START/ON/RESET。 */
    if (command_words[3] & 1U)
    {
        SstCommand_EmergencyStop();
        ChbDabControl_MaintenanceSwitch(0U, 3U);
        return SST_PARAM_OK;
    }
    /* 第3步：word9 / 00F9有OFF时，只提交本包OFF并返回。
     * bit0在SstControlCommand_ApplyRequests内清dab_enable；bit1清run_enable。
     * 保留既有OFF优先规则；其他START/ON动作应下一笔单独发送。 */
    if (command_words[9] & 3U)
    {
        SstControlCommand_ApplyRequests(0U, 0U, 0U, command_words[9], 0U);
        return SST_PARAM_OK;
    }
    /* 第4步：只针对已提取的本机请求检查存储忙、许可及故障联锁。
     * 任一检查失败即返回；最终提交前不会部分置1run_enable或dab_enable。 */
    if (SstParams_IsStorageBusy())
    {
        return SST_PARAM_BUSY;
    }
    if (command_words[11] == 3U) //bit0允许合闸 bit1禁止合闸 两位同时1拒绝
    {
        return SST_PARAM_RANGE;
    }
    permission = g_sst_dpram_screen_result.switch_permission;
    if (command_words[11] == 1U)//bit0==1
    {
        permission = 1U;
    }
    if (command_words[11] == 2U)//bit1==1
    {
        permission = 0U;
    }
    if ((command_words[2] || command_words[8]) &&//故障不允许启动
        (g_chb_dab_status.fault_code != CHB_FAULT_NONE || g_chb_dab_status.state == CHB_STATE_FAULT))
    {
        return SST_PARAM_DENIED;
    }
    if ((command_words[10] != 0U) && (SstCommand_IsStopped() == 0U))//未完全停机不允许复位
    {
        return SST_PARAM_BUSY;
    }
    if ((command_words[6] != 0U) || (command_words[7] != 0U))
    {
        /* 手动方式才允许直接操作QF1/KT1；自动启动由预充状态机操作。
         * 00F6/00F7 bit0是KT1、bit1是QF1；这是屏幕协议位，不是DO位号。
         * MaintenanceSwitch调用Chb_SetQf1/Chb_SetKt1，再由Port_IO_Out写S301。 */
        if ((PortRunFLAG.Bbits.Port_AutoOrManual != 0U) || //1自动 0手动
            ((command_words[6] != 0U) && (permission == 0U)))
        {
            return SST_PARAM_DENIED;
        }
        index = ChbDabControl_MaintenanceSwitch(command_words[6], command_words[7]);
        if (index != SST_PARAM_OK)
        {
            return index;
        }
    }
    g_sst_dpram_screen_result.switch_permission = permission;
    /* 第5步：校验通过才提交实际请求。五个实参依次为START、STOP、ON、OFF、RESET。
     * START：word2 / 00F2 bit0 -> SstControlCommand_ApplyRequests内run_enable=1；DAB不变。
     * ON：word8 / 00F8 bit0 -> dab_enable=1；bit1 -> run_enable=1。
     * RESET：word10 / 00FA bit0 -> 完全停机时fault_reset=1。
     * STOP/OFF已在前面优先处理，所以这里两个对应实参为0。
     * 命令提交后主循环的ChbDabControl_Service及10kHz FastStep决定实际PWM。
     * 遥测1985=0表示本次受理，不代表198F的实际PWM位已经置1。 */
    SstControlCommand_ApplyRequests(command_words[2], 0U, command_words[8], 0U, command_words[10]);
    return SST_PARAM_OK;
}
