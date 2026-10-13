// 文件职责：原软件保护与逐模块电压巡检的受控参数入口，供DPRAM及持久化共用。
#include "SstLegacyParameters.h"
#include "SstParams.h"
#include "ProtectionData.h"

/*==================================================================
 * 函数功能: 读取原保护/电压巡检参数并转换为16位线上编码
 * 输入参数: index：0一级OC阈值、1计时、2二级OC阈值、3计时、4OV阈值、5计时、6预充巡检低压、7巡检高压
 * 输出参数: 无
 * 返回值: 前六项原整数；后两项0.1V/count，越界索引返回0
 * 执行说明: 后台调用，电压编码饱和，不访问硬件或改变10kHz时序。
 *================================================================*/
uint16_t SstLegacyParameters_Read(uint16_t index)
{
    float scaled; /* scaled：巡检电压换算后的线上值，处理调试器可能写入的非法值。 */
    switch(index)
    {
        case 0: return OC_level_SET;
        case 1: return OC_time_SET;
        case 2: return OC_level_SET_L2;
        case 3: return OC_time_SET_L2;
        case 4: return OV_level_SET;
        case 5: return OV_time_SET;
        case 6: scaled = HA_L_Voltage_Precharge * 10.0F; break;
        case 7: scaled = HA_H_Voltage * 10.0F; break;
        default: return 0U;
    }
    if(scaled != scaled || scaled <= 0.0F) return 0U;
    if(scaled >= 65535.0F) return 65535U;
    return (uint16_t)(scaled + 0.5F);
}

/*==================================================================
 * 函数功能: 校验原保护及模块电压巡检的完整八字镜像
 * 输入参数: snapshot：八个按Read顺序排列的参数字
 * 输出参数: 无
 * 返回值: SST_PARAM_OK/BAD_ADDRESS/RANGE
 * 执行说明: 保护电流及过压阈值不得为0；计时0..65534，拒绝16位计数无法严格超过的65535。
 *================================================================*/
uint16_t SstLegacyParameters_ValidateSnapshot(const uint16_t *snapshot)
{
    if(!snapshot) return SST_PARAM_BAD_ADDRESS;
    if(!snapshot[0] || !snapshot[2] || !snapshot[4] ||
       snapshot[1] == 65535U || snapshot[3] == 65535U || snapshot[5] == 65535U ||
       !snapshot[7] || snapshot[6] >= snapshot[7]) return SST_PARAM_RANGE;
    return SST_PARAM_OK;
}

/*==================================================================
 * 函数功能: 应用已整体校验的原保护/电压巡检镜像
 * 输入参数: snapshot：八个有效参数字，调用者已校验并保证停机
 * 输出参数: 原OC/OV阈值与计时、HA_L_Voltage_Precharge及HA_H_Voltage
 * 返回值: 无
 * 执行说明: 后台内部恢复入口，不操作PWM、FPGA及中断。
 *================================================================*/
void SstLegacyParameters_ApplySnapshot(const uint16_t *snapshot)
{
    OC_level_SET = snapshot[0]; OC_time_SET = snapshot[1];
    OC_level_SET_L2 = snapshot[2]; OC_time_SET_L2 = snapshot[3];
    OV_level_SET = snapshot[4]; OV_time_SET = snapshot[5];
    HA_L_Voltage_Precharge = (float)snapshot[6] / 10.0F;
    HA_H_Voltage = (float)snapshot[7] / 10.0F;
}

/*==================================================================
 * 函数功能: 在停机与保存互锁下修改单项原保护/巡检参数
 * 输入参数: index：0..7；value：Read定义的16位编码
 * 输出参数: 校验通过后应用完整八字参数镜像
 * 返回值: SST_PARAM_*；失败时不修改参数
 * 执行说明: 后台维护入口；不允许运行期间降低保护阈值或修改巡检门槛。
 *================================================================*/
uint16_t SstLegacyParameters_Write(uint16_t index, uint16_t value)
{
    uint16_t snapshot[SST_LEGACY_PARAMETER_COUNT]; /* snapshot：原参数快照，用于跨阈值校验。 */
    uint16_t i; /* i：当前数组、快照或点表遍历下标。 */
    uint16_t result; /* result：镜像校验结果。 */
    if(index >= SST_LEGACY_PARAMETER_COUNT) return SST_PARAM_BAD_ADDRESS;
    if(!SstParams_IsStopped() || SstParams_IsStorageBusy()) return SST_PARAM_BUSY;
    for(i = 0U; i < SST_LEGACY_PARAMETER_COUNT; ++i) snapshot[i] = SstLegacyParameters_Read(i);
    snapshot[index] = value;
    result = SstLegacyParameters_ValidateSnapshot(snapshot);
    if(result == SST_PARAM_OK) SstLegacyParameters_ApplySnapshot(snapshot);
    return result;
}
