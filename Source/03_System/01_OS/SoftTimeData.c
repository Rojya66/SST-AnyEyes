/* 文件职责: 原 Timer2 软件等待、节拍和指示标志；从原主文件迁移，符号、类型、初值及硬件地址保持。 */
#include "SoftTimeData.h"
#include "SstControlConfig.h"

unsigned char Flag_testT; /* Flag_testT：原测试定时观察标志。 */

unsigned char Flag_temp; /* Flag_temp：后台运行指示灯翻转状态。 */

unsigned char Flag_WDI; /* Flag_WDI：后台外部看门狗 GPIO 翻转状态。 */

unsigned char Flag_WaitPowerStart10s; /* Flag_WaitPowerStart10s：Timer2 更新、后台消费的主电源等待 10 s到时标志。 */

unsigned char Flag_1s; /* Flag_1s：Timer2 更新、后台消费的周期 1 s到时标志。 */

unsigned char Flag_50ms; /* Flag_50ms：Timer2 更新、后台消费的周期 50 ms到时标志。 */

unsigned char Flag_100ms; /* Flag_100ms：Timer2 更新、后台消费的周期 100 ms到时标志。 */

unsigned char Flag_RemoteMeasure; /* Flag_RemoteMeasure：Timer2 更新、后台消费的遥测轮询到时标志。 */

unsigned char Flag_Switch500ms; /* Flag_Switch500ms：Timer2 更新、后台消费的开关操作等待 500 ms到时标志。 */

unsigned char Flag_Switch1s; /* Flag_Switch1s：Timer2 更新、后台消费的开关操作等待 1 s到时标志。 */

unsigned char Flag_Precharge500ms; /* Flag_Precharge500ms：Timer2 更新、后台消费的预充等待 500 ms到时标志。 */

unsigned char Flag_PreStart3s; /* Flag_PreStart3s：Timer2 更新、后台消费的启动准备等待 3 s到时标志。 */

unsigned char Flag_20s; /* Flag_20s：Timer2 更新、后台消费的上电等待 20 s到时标志。 */

unsigned char Flag_Wait; /* Flag_Wait：原控制流程等待标志。 */

unsigned char Flag_StopWait; /* Flag_StopWait：原停机延时等待标志。 */

unsigned char Flag_StopWaitEnd; /* Flag_StopWaitEnd：原停机等待结束标志。 */

unsigned short
    countWaitPowerStart10s; /* countWaitPowerStart10s：Timer2 原 主电源等待 10 s计数；实际节拍沿用原判断式。 */

unsigned short cout_Wait; /* cout_Wait：Timer2 原 原控制等待计数；实际节拍沿用原判断式。 */

unsigned short cout200us; /* cout200us：Timer2 原 原名为 200 us 的兼容定时计数；实际节拍沿用原判断式。 */

unsigned short cout1s; /* cout1s：Timer2 原 周期 1 s计数；实际节拍沿用原判断式。 */

unsigned short cout50ms; /* cout50ms：Timer2 原 周期 50 ms计数；实际节拍沿用原判断式。 */

unsigned short cout100ms; /* cout100ms：Timer2 原 周期 100 ms计数；实际节拍沿用原判断式。 */

unsigned short coutRemoteMeasure; /* coutRemoteMeasure：Timer2 原 遥测轮询计数；实际节拍沿用原判断式。 */

unsigned short coutStopWait; /* coutStopWait：Timer2 原 停机等待计数；实际节拍沿用原判断式。 */

unsigned short coutSwitch500ms; /* coutSwitch500ms：Timer2 原 开关操作等待 500 ms计数；实际节拍沿用原判断式。 */

unsigned short coutSwitch1s; /* coutSwitch1s：Timer2 原 开关操作等待 1 s计数；实际节拍沿用原判断式。 */

unsigned short coutPrecharge500ms; /* coutPrecharge500ms：Timer2 原 预充等待 500 ms计数；实际节拍沿用原判断式。 */

unsigned short coutPreStart3s; /* coutPreStart3s：Timer2 原 启动准备等待 3 s计数；实际节拍沿用原判断式。 */

unsigned short coutSVG1s; /* coutSVG1s：Timer2 原 SVG 投入等待 1 s计数；实际节拍沿用原判断式。 */

unsigned short cout20s; /* cout20s：Timer2 原 上电等待 20 s计数；实际节拍沿用原判断式。 */

unsigned char TestINT; /* TestINT：原中断调试观察标志。 */
