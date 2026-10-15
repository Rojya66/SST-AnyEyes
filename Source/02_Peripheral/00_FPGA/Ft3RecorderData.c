#include "SstTypes.h"
/* 文件职责: FT3 映射、录波状态和采样序号；从原主文件迁移，符号、类型、初值及硬件地址保持。 */
#include "Ft3RecorderData.h"
#include "SstControlConfig.h"
#pragma DATA_SECTION(FT3_PTCT_Register, ".FT3_PTCT_Register");
#pragma DATA_SECTION(FT3_FaultRecorder_Register, ".FT3_FaultRecorder_Register");
#pragma DATA_SECTION(FT3_EER2_Register, ".FT3_EER2_Register");
#pragma DATA_SECTION(FT3_Backup_Register, ".FT3_Backup_Register");

volatile XXG_FaultRecordStatus FaultRecordStatus; /* FaultRecordStatus：FT3 故障录波主运行模式状态字。 */

volatile XXG_FaultRecordStatus_1 FaultRecordStatus_1; /* FaultRecordStatus_1：FT3 故障录波谐波/STATCOM 模式状态字。 */

volatile XXG_FT3_Register FT3_PTCT_Register, /* FT3_PTCT_Register：PT/CT FT3 插件的原映射寄存器窗口。 */
    FT3_FaultRecorder_Register, /* FT3_FaultRecorder_Register：故障录波 FT3 插件的原映射寄存器窗口。 */
    FT3_EER2_Register, /* FT3_EER2_Register：EER2 FT3 插件的原映射寄存器窗口。 */
    FT3_Backup_Register; /* FT3_Backup_Register：备用 FT3 插件的原映射寄存器窗口。 */

ushort faultsignal; /* faultsignal：原 FT3 故障触发信号。 */

ushort faultRecordStart; /* faultRecordStart：原 FT3 录波启动标志。 */

ushort fault_count; /* fault_count：原故障录波触发计数。 */

ushort fault_count_start; /* fault_count_start：原故障录波开始计数/状态。 */

ushort FR_tri_count1, /* FR_tri_count1：原 FT3 录波第 1 组触发计数。 */
    FR_tri_count2, /* FR_tri_count2：原 FT3 录波第 2 组触发计数。 */
    FR_tri_count3; /* FR_tri_count3：原 FT3 录波第 3 组触发计数。 */

unsigned short FT3_Samp_Cnt; /* FT3_Samp_Cnt：FT3 上报的采样序号。 */

unsigned short dataPackNum; /* dataPackNum：FT3 数据包数量/控制字字段。 */
