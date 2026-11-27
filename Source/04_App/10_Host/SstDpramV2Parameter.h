#ifndef SST_DPRAM_V2_PARAMETER_H
#define SST_DPRAM_V2_PARAMETER_H

#include "SstParamTypes.h"

#define SST_DPRAM_V2_PARAMETER_BASE 0x0200U
#define SST_DPRAM_V2_PARAMETER_WORDS 63U

/* 按0x0200..0x023E的顺序编码活动参数。 */
void SstDpramV2Parameter_Read(uint16_t *words);
/* 完全停机时校验整个候选，再应用变化项。错误位置使用DPRAM字地址。 */
uint16_t SstDpramV2Parameter_Apply(const uint16_t *words, uint16_t *error_addr);
/* load为0时启动异步保存，为1时加载；校验失败可返回对应DPRAM字地址。 */
uint16_t SstDpramV2Parameter_Store(uint16_t load, uint16_t *error_addr);

#endif
