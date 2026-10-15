/* 文件职责: 模块接口。数据只在所属 .c 定义，外部通过本头的必要声明访问。 */
#ifndef MAIN_H_
#define MAIN_H_
#include "ControlAlgorithmTypes.h"
#include "ControlData.h"
#include "ControlInit.h"
#include "DpramConfig.h"
#include "DpramHostData.h"
#include "DpramRegister.h"
#include "FpgaRegister.h"
#include "Ft3RecorderData.h"
#include "LegacySerialData.h"
#include "MeasurementData.h"
#include "ModuleVoltageCheck.h"
#include "PortStatusData.h"
#include "PortStatusTypes.h"
#include "ProtectionData.h"
#include "ProtectionTypes.h"
#include "Redef_Interrupt.h"
#include "SoftTimeData.h"
#include "SstBoard.h"
#include "SstBoardConfig.h"
#include "SstControlConfig.h"
#include "SstTypes.h"
#include "TransformData.h"

#ifdef __cplusplus
extern "C"
{
#endif
    /* 仅供历史代码过渡；活动模块直接包含所属接口。 */

#ifdef __cplusplus
}
#endif
#endif
