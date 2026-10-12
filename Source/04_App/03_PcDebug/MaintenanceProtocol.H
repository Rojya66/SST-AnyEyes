#ifndef __MAINTENANCE_PROTOCOL_H__
#define __MAINTENANCE_PROTOCOL_H__

#ifdef __cplusplus
extern "C" {
#endif

/* MAINT_SOF0：维护帧头第一字节。 */
#define MAINT_SOF0              0xA5
/* MAINT_SOF1：维护帧头第二字节。 */
#define MAINT_SOF1              0x5A
/* MAINT_VERSION：维护线上帧版本编号。 */
#define MAINT_VERSION           0x01
/* MAINT_MAX_PAYLOAD：维护帧允许的最大载荷字节数。 */
#define MAINT_MAX_PAYLOAD       240
/* MAINT_RX_QUEUE_SIZE：维护字节接收环容量。 */
#define MAINT_RX_QUEUE_SIZE     512
/* MAINT_UART_BAUD：旧维护协议串口波特率兼容定义。 */
#define MAINT_UART_BAUD         115200

/* MAINT_FLAG_RESPONSE：维护帧响应标志位。 */
#define MAINT_FLAG_RESPONSE     0x80
/* MAINT_FLAG_ERROR：维护帧错误响应标志位。 */
#define MAINT_FLAG_ERROR        0x40

/* MAINT_CMD_HELLO：维护连接信息查询命令。 */
#define MAINT_CMD_HELLO         0x01
/* MAINT_CMD_READ_OBJECT：维护对象读取命令。 */
#define MAINT_CMD_READ_OBJECT   0x02
/* MAINT_CMD_WRITE_OBJECT：维护对象写入命令。 */
#define MAINT_CMD_WRITE_OBJECT  0x03
/* MAINT_CMD_EXEC_CONTROL：维护遥控执行命令。 */
#define MAINT_CMD_EXEC_CONTROL  0x04
/* MAINT_CMD_SAVE：维护参数保存命令。 */
#define MAINT_CMD_SAVE          0x05

/* MAINT_OBJ_STATUS：维护固定状态对象编号。 */
#define MAINT_OBJ_STATUS        0x01
/* MAINT_OBJ_TELEMETRY：维护端口遥测对象编号。 */
#define MAINT_OBJ_TELEMETRY     0x02
/* MAINT_OBJ_CONTROL：维护遥控缓存对象编号。 */
#define MAINT_OBJ_CONTROL       0x03
/* MAINT_OBJ_REGULATE：维护遥调缓存对象编号。 */
#define MAINT_OBJ_REGULATE      0x04
/* MAINT_OBJ_PLUGIN：维护原始插件对象编号。 */
#define MAINT_OBJ_PLUGIN        0x05
/* MAINT_OBJ_PARAMETER：维护单个旧参数对象编号。 */
#define MAINT_OBJ_PARAMETER     0x06
/* Read-only aggregate objects used by the maintenance page refresh. */
/* MAINT_OBJ_ALL_PARAMETERS：维护全部旧参数对象编号。 */
#define MAINT_OBJ_ALL_PARAMETERS 0x07
/* MAINT_OBJ_PLUGIN_SUMMARY：维护插件摘要对象编号。 */
#define MAINT_OBJ_PLUGIN_SUMMARY 0x08

/* MAINT_CTL_START：维护端口启动控制选择。 */
#define MAINT_CTL_START         0x01
/* MAINT_CTL_STOP：维护端口停止控制选择。 */
#define MAINT_CTL_STOP          0x02
/* MAINT_CTL_MODE：维护旧端口模式设置控制选择。 */
#define MAINT_CTL_MODE          0x03
/* MAINT_CTL_SWITCH_ON：维护开关闭合控制选择。 */
#define MAINT_CTL_SWITCH_ON     0x04
/* MAINT_CTL_SWITCH_OFF：维护开关断开控制选择。 */
#define MAINT_CTL_SWITCH_OFF    0x05
/* MAINT_CTL_HF_ON：维护 DAB 高频桥使能控制选择。 */
#define MAINT_CTL_HF_ON         0x06
/* MAINT_CTL_LF_ON：维护 CHB 工频桥使能控制选择。 */
#define MAINT_CTL_LF_ON         0x07
/* MAINT_CTL_HF_OFF：维护 DAB 高频桥禁用控制选择。 */
#define MAINT_CTL_HF_OFF        0x08
/* MAINT_CTL_LF_OFF：维护 CHB 工频桥禁用控制选择。 */
#define MAINT_CTL_LF_OFF        0x09
/* MAINT_CTL_FAULT_CLEAR：维护故障清除控制选择。 */
#define MAINT_CTL_FAULT_CLEAR   0x0A
/* MAINT_CTL_SWITCH_EN：维护开关动作允许控制选择。 */
#define MAINT_CTL_SWITCH_EN     0x0B
/* MAINT_CTL_SWITCH_DIS：维护开关动作禁止控制选择。 */
#define MAINT_CTL_SWITCH_DIS    0x0C

/* MAINT_ERR_OK：维护操作成功结果码。 */
#define MAINT_ERR_OK            0x00
/* MAINT_ERR_BAD_VERSION：维护版本不支持错误码。 */
#define MAINT_ERR_BAD_VERSION   0x01
/* MAINT_ERR_BAD_LENGTH：维护载荷长度非法错误码。 */
#define MAINT_ERR_BAD_LENGTH    0x02
/* MAINT_ERR_BAD_CRC：维护 CRC 校验失败错误码。 */
#define MAINT_ERR_BAD_CRC       0x03
/* MAINT_ERR_BAD_COMMAND：维护命令不支持错误码。 */
#define MAINT_ERR_BAD_COMMAND   0x04
/* MAINT_ERR_BAD_OBJECT：维护对象不支持错误码。 */
#define MAINT_ERR_BAD_OBJECT    0x05
/* MAINT_ERR_BAD_RANGE：维护对象范围/数值非法错误码。 */
#define MAINT_ERR_BAD_RANGE     0x06
/* MAINT_ERR_NOT_ALLOWED：维护权限拒绝错误码。 */
#define MAINT_ERR_NOT_ALLOWED   0x07
/* MAINT_ERR_BUSY：维护共享存储忙/非停机错误码。 */
#define MAINT_ERR_BUSY          0x08
/* MAINT_ERR_EEPROM：维护 EEPROM 校验/写入失败错误码。 */
#define MAINT_ERR_EEPROM        0x09

/*==================================================================
 * 函数功能: 清空维护接收队列、帧解析及持久化任务状态
 * 输入参数: 无
 * 输出参数: 维护接收/响应缓存、兼容参数或页 4/5 存储镜像；启停仍由控制核心执行。
 * 返回值: 无
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
void MaintenanceProtocol_Init(void);
/*==================================================================
 * 函数功能: 将接收字节放入维护协议环形队列
 * 输入参数: value：本次协议/存储处理的值，类型和编码保持原接口约定
 * 输出参数: 维护接收/响应缓存、兼容参数或页 4/5 存储镜像；启停仍由控制核心执行。
 * 返回值: 无
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
void MaintenanceProtocol_RxByte(unsigned char value);
/* UART2接收分流：返回1表示该字节属于维护帧，不再交给旧协议解析器。 */
/*==================================================================
 * 函数功能: 检查维护帧头与长度以决定当前字节是否属于维护协议
 * 输入参数: value：本次协议/存储处理的值，类型和编码保持原接口约定
 * 输出参数: 维护接收/响应缓存、兼容参数或页 4/5 存储镜像；启停仍由控制核心执行。
 * 返回值: 非零表示字节已被维护协议接收
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
unsigned char MaintenanceProtocol_RxByteForUart(unsigned char value);
/*==================================================================
 * 函数功能: 在接收预算内推进维护帧解析及逐字节参数保存
 * 输入参数: 无
 * 输出参数: 维护接收/响应缓存、兼容参数或页 4/5 存储镜像；启停仍由控制核心执行。
 * 返回值: 无
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
void MaintenanceProtocol_Process(void);
/*==================================================================
 * 函数功能: 查询维护协议参数保存任务是否仍在执行
 * 输入参数: 无
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 非零表示维护保存忙
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
unsigned short MaintenanceProtocol_IsSaving(void);
/*==================================================================
 * 函数功能: 读取页 4/5 维护镜像并在校验通过时恢复旧参数
 * 输入参数: 无
 * 输出参数: 维护接收/响应缓存、兼容参数或页 4/5 存储镜像；启停仍由控制核心执行。
 * 返回值: 无
 * 执行说明: SST_ENABLE_MAINTENANCE 可选后台兼容路径；不会把旧 UART ISR 注册到新的调试收发链路。
 *================================================================*/
void MaintenanceProtocol_LoadPersistent(void);

#ifdef __cplusplus
}
#endif

#endif
