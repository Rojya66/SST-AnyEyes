#include "SstDpramHost.h"
#include "ChbBuildConfig.h"
#include "DpramHostData.h"
#include "DpramRegister.h"
#include "DpramConfig.h"
#include "FpgaRegister.h"
#include "SoftTimeData.h"
#include "SstControlConfig.h"
#include "SstBoardConfig.h"
#include "DPRAM.h"
#include "SstCommand.h"
#include "SstParams.h"
#include "SstDpramMaintenance.h"
#include "SstDpramProtocol.h"
#include "ControlData.h"

static unsigned short s_regulate_reply_sent; /* 邮箱20刚发布，本轮遥测让出通知窗口。 */

/*==================================================================
 * 函数功能: 读取可选 DPRAM 遥控/遥调块并交给统一命令入口
 * 输入参数: 无
 * 输出参数: 消费遥控通知；读取旧控制/遥调缓存，提交统一启停命令或标记扩展维护请求。
 * 返回值: 无
 * 执行说明: 仅由主循环调用；SST_ENABLE_DPRAM=0 时不访问并口。
 *================================================================*/
void SstDpramHost_ProcessIncomingBlocks(void)
{
#if SST_ENABLE_DPRAM
    if (Flag_RemoteControlRegulate == 0U)
    {
        return;
    }

    Flag_RemoteControlRegulate = 0;
    rtt = (rtt == 0U) ? 1U : 0U;
    FPGA_Interface[MainControl_PluginUnit_4 - 2].LED.Bbits.led3 = rtt;

    /* 接收第1步：DPRAM_Read读取左邮箱1FFE，并按邮箱号复制共享区到本机RAM。
     * 邮箱1：00F0..00FF的16字 -> PORT_Control_DATA_Buf.DPRAM_Block1Buf。
     * 邮箱2：01FF..0229的43字 -> PORT_Regulate_DATA_Buf.DPRAM_Block2Buf。
     * 这里只完成通知及数据复制，尚未赋值run_enable、dab_enable或活动PI。 */
    DPRAM_Read();
    if (FLAG_NEW_L == SST_DPRAM_MAINTENANCE_BLOCK)
    {
        SstDpramMaintenance_MarkRequestPending(); // 邮箱21：现代参数维护，后台分片回复
    }
    else if (FLAG_NEW_L == ReadBlock_remControl)
    {
        /* 接收第2步A：解析16字掩码、检查联锁，最后经SstControlCommand_ApplyRequests赋值。
         * 00F2 bit0仅请求CHB；00F8/00F9 bit0单独请求DAB ON/OFF。
         * control_result由屏幕1985读，control_count由1984读；不以Modbus ACK判成功。 */
        g_sst_dpram_screen_result.control_result = SstDpramCommand_ProcessControlBlock(PORT_Control_DATA_Buf.DPRAM_Block1Buf);
        ++g_sst_dpram_screen_result.control_count;
    }
    else if (FLAG_NEW_L == ReadBlock_remRegulate)
    {
        /* 接收第2步B：原43字 -> 检查本块字段 -> 直接赋值Udc、频率、PI及保护使能。
         * 最终赋值在SstDpramParameter_DecodeAndApplyLegacyBlock中按地址逐项明写。
         * 本路径不写run_enable/dab_enable；01FF的运行/测试位只是管理方式。 */
        g_sst_dpram_screen_result.regulate_result =
            SstDpramParameter_DecodeAndApplyLegacyBlock(PORT_Regulate_DATA_Buf.DPRAM_Block2Buf);
        ++g_sst_dpram_screen_result.regulate_count;
        /* 接收第3步：把实际生效参数重新编码回原43字，再回写DPRAM并通知邮箱20。
         * 失败也回实际值，不能把候选回显当成功；读1986计数、1987结果确认本笔。 */
        SstDpramParameter_EncodeLegacyBlock(PORT_Regulate_DATA_Buf.DPRAM_Block2Buf);
        /* 旧DPRAM_Write(20)只有此标志为1才复制43字并发右邮箱20；写完会清0。
         * 每笔成功/失败遥调都必须重新置位，回传实际值，避免只在上电回读一次。 */
        FLAG_RemRegulate_SetOver = 1U;
        DPRAM_Write(WriteBlock_remRegulate);
        s_regulate_reply_sent = 1U;
    }
#else
    /* SST_ENABLE_DPRAM=0 时跳过并口控制。 */
    return;
#endif
}

/*==================================================================
 * 函数功能: 按轮询顺序回写可选 DPRAM 端口遥测及有效插件块
 * 输入参数: 无
 * 输出参数: DPRAM 端口遥测块及本轮插件/遥调块，更新 WriteBlock_Index。
 * 返回值: 无
 * 执行说明: 主循环消费遥测节拍标志；每次回写公共块和一个轮询块。
 *================================================================*/
void SstDpramHost_PublishTelemetry(void)
{
#if SST_ENABLE_DPRAM
    if (Flag_RemoteMeasure == 0U)
    {
        return;
    }

    Flag_RemoteMeasure = 0;
    if (s_regulate_reply_sent)
    {
        s_regulate_reply_sent = 0U;
        return; /* 避免刚发邮箱20就在同一轮主循环被遥测通知覆盖。 */
    }
    SstDpramMaintenance_PublishSnapshots();
    SstDpramScreen_PublishStatus();
    /* 一个30ms节拍只发布一个旧邮箱，避免块3刚通知就被插件覆盖。
     * 插件数据仍是固定区可轮询镜像；邮箱无队列/ACK，不宣称绝不漏通知。 */
    static unsigned short publish_common = 1U; /* 交替发布块3与插件的后台标志。 */
    if (publish_common)
    {
        DPRAM_Write(WriteBlock_remMeaSig);
        publish_common = 0U;
    }
    else
    {
        unsigned short count; /* 本轮最多检查16个插件槽位，不忙等。 */
        unsigned short valid; /* 配置中有效的数据生产者位图。 */
        valid = SstDpramTelemetry_GetPluginMask();
        for (count = 0U; count < 16U; ++count)
        {
            if (WriteBlock_Index < WriteBlock_PluginUnit_2 || WriteBlock_Index >= WriteBlock_PluginUnit_17)
                WriteBlock_Index = WriteBlock_PluginUnit_2;
            if (valid & (1U << (WriteBlock_Index - WriteBlock_PluginUnit_2)))
            {
                DPRAM_Write(WriteBlock_Index++);
                break;
            }
            ++WriteBlock_Index;
        }
        publish_common = 1U;
    }
#else
    /* SST_ENABLE_DPRAM=0 时跳过并口遥测。 */
    return;
#endif
}

/*==================================================================
 * 函数功能: 开放DPRAM通知前发布原块2实际初值
 * 输入参数: 无
 * 输出参数: 原块2实际参数及邮箱20；周期任务不再覆写主机正在编辑的块2
 * 返回值: 无
 * 执行说明: 仅初始化调用；后续只有接收邮箱2才回写该共享区域。
 *================================================================*/
void SstDpramHost_Initialize(void)
{
#if SST_ENABLE_DPRAM
    SstDpramParameter_EncodeLegacyBlock(PORT_Regulate_DATA_Buf.DPRAM_Block2Buf);
    /* 与每笔遥调回复使用相同的旧回写许可；DPRAM_Write完成后自行清零。 */
    FLAG_RemRegulate_SetOver = 1U;
    DPRAM_Write(WriteBlock_remRegulate);
    WriteBlock_Index = WriteBlock_PluginUnit_2;
    s_regulate_reply_sent = 0U;
#endif
}
