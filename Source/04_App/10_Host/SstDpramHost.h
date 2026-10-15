/* 文件职责: SstDpramHost.h 模块公开接口。 */
#ifndef SSTDPRAMHOST_H_
#define SSTDPRAMHOST_H_
#ifdef __cplusplus
extern "C"
{
#endif
    /*==================================================================
 * 函数功能: 消费可选DPRAM控制通知并提交统一控制命令
 * 输入参数: 无
 * 输出参数: 消费遥控通知；读取旧控制/遥调缓存，提交统一启停命令或标记扩展维护请求。
 * 返回值: 无
 * 执行说明: 仅由主循环调用；SST_ENABLE_DPRAM=0 时不访问并口。
 *================================================================*/
    void SstDpramHost_ServiceRemoteControl(void);
    /*==================================================================
 * 函数功能: 有界轮询可选DPRAM端口和插件遥测块
 * 输入参数: 无
 * 输出参数: DPRAM 端口遥测块及本轮插件/遥调块，更新 WriteBlock_Index。
 * 返回值: 无
 * 执行说明: 主循环消费遥测节拍标志；每次回写公共块和一个轮询块。
 *================================================================*/
    void SstDpramHost_ServiceTelemetry(void);
#ifdef __cplusplus
}
#endif
#endif
