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

/*==================================================================
 * 函数功能: 读取可选 DPRAM 遥控/遥调块并交给统一命令入口
 * 输入参数: 无
 * 输出参数: 消费遥控通知；读取旧控制/遥调缓存，提交统一启停命令或标记扩展维护请求。
 * 返回值: 无
 * 执行说明: 仅由主循环调用；SST_ENABLE_DPRAM=0 时不访问并口。
 *================================================================*/
void SstDpramHost_ServiceRemoteControl(void)
{
#if SST_ENABLE_DPRAM
    if (!Flag_RemoteControlRegulate)
    {
        return;
    }

    Flag_RemoteControlRegulate = 0;
    rtt = !rtt;
    FPGA_Interface[MainControl_PluginUnit_4 - 2].LED.Bbits.led3 = rtt;

    DPRAM_Read(); // 读取上位机写入的控制块
    if (FLAG_NEW_L == SST_DPRAM_MAINTENANCE_BLOCK)
    {
        SstDpramMaintenance_Request(); // 邮箱21：现代参数维护，后台分片回复
    }
    else if (FLAG_NEW_L == ReadBlock_remControl)
    {
        /* Preserve the DPRAM addresses; all power gates belong to the core. */
        SstCommand_ApplyDpram(PORT_Control_DATA_Buf.DPRAM_RemControl.COM_START_PORT.COM_START_PORT16,
                              PORT_Control_DATA_Buf.DPRAM_RemControl.COM_STOP_PORT.COM_STOP_PORT16,
                              PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PLUSE_ON.COM_PLUSE16,
                              PORT_Control_DATA_Buf.DPRAM_RemControl.COM_PLUSE_OFF.COM_PLUSE16,
                              PORT_Control_DATA_Buf.DPRAM_RemControl.COM_FAULT_CLEAR.COM_FAULT_CLEAR16);
    }
#if SST_ALLOW_REMOTE_COMMANDS
    else if ((FLAG_NEW_L == ReadBlock_remRegulate) && SstParams_IsStopped() && !SstParams_IsStorageBusy())
    {
        RemoteRegulate_Dispose_2(0);
    }
#endif
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
void SstDpramHost_ServiceTelemetry(void)
{
#if SST_ENABLE_DPRAM
    if (!Flag_RemoteMeasure)
    {
        return;
    }

    Flag_RemoteMeasure = 0;
    SstDpramMaintenance_Publish(); // 只构建RAM快照，新增并口访问另行分片
    DPRAM_Write(WriteBlock_remMeaSig); // 先回写端口公共状态

    if (WriteBlock_Index < WriteBlock_PluginUnit_7)
    {
        DPRAM_Write(WriteBlock_Index++);
        if (WriteBlock_Index == WriteBlock_PluginUnit_7)
        {
            WriteBlock_Index++;
        }
    }
    else if (WriteBlock_Index < WriteBlock_PluginUnit_11)
    {
        DPRAM_Write(WriteBlock_Index++);
        if (WriteBlock_Index == WriteBlock_PluginUnit_11)
        {
            WriteBlock_Index++;
        }
    }
    else if (WriteBlock_Index < WriteBlock_PluginUnit_15)
    {
        DPRAM_Write(WriteBlock_Index++);
        if (WriteBlock_Index == WriteBlock_PluginUnit_15)
        {
            WriteBlock_Index++;
        }
    }
    else if (WriteBlock_Index == WriteBlock_PluginUnit_17)
    {
        DPRAM_Write(WriteBlock_Index++);
    }
    else if (WriteBlock_Index == WriteBlock_remRegulate)
    {
        DPRAM_Write(WriteBlock_Index);
        WriteBlock_Index = WriteBlock_PluginUnit_2;
    }
    else
    {
        /* 每次只发送一个插件块，避免主循环被连续EMIFA访问长时间占用。 */
        DPRAM_Write(WriteBlock_Index++);
    }
#else
    /* SST_ENABLE_DPRAM=0 时跳过并口遥测。 */
    return;
#endif
}
