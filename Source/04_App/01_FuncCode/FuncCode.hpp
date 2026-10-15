#ifndef SST_FUNC_CODE_HPP
#define SST_FUNC_CODE_HPP
#include <stdint.h>
#include "SstParams.h"
typedef uint16_t u16;
typedef int16_t s16;
/* ALL_LIST_NUM：参考功能码组数量，保持固定 30 组。 */
#define ALL_LIST_NUM SST_PARAM_GROUPS
/* SINGLE_LIST_NUM：参考每组参数数量，保持固定 34 字。 */
#define SINGLE_LIST_NUM SST_PARAM_PER_GROUP

/* Retained reference class and access signatures. Storage is an explicit action. */
class C_FUNC_CODE
{
  public:
    /*==================================================================
     * 函数功能: 把参考 FuncCode 读接口转接到 SST 固定参数地址表
     * 输入参数: group：固定参数组号，范围 0..29
     *           index：所选表/对象的索引；范围按函数对应表容量检查
     *           value：输出指针，用于返回固定地址对应的 16 位线上参数编码
     * 输出参数: value 指向的一个 16 位参数值。
     * 返回值: 成功返回 1，失败返回负的 SST_PARAM_* 结果
     * 执行说明: 后台固定参数表访问，不接受任意 RAM 地址。
     *================================================================*/
    s16 GetOneValueFromRealMem(u16 group, u16 index, u16 *value)
    {
        u16 result = SstParams_Read(
            group, index, value); /* result：SstParams 返回的 SST_PARAM_* 状态，供参考接口转换成功/失败返回值。 */
        return result ? -(s16)result : 1;
    }

    /*==================================================================
     * 函数功能: 把参考 FuncCode 写接口转接到 SST 参数校验及写入服务
     * 输入参数: group：固定参数组号，范围 0..29
     *           index：所选表/对象的索引；范围按函数对应表容量检查
     *           value：待写入的 16 位线上参数编码，有符号量按二补码解释
     * 输出参数: 更新固定映射地址对应的受控成员；保存须单独发出请求。
     * 返回值: 成功返回 1，失败返回负的 SST_PARAM_* 结果
     * 执行说明: 后台固定参数表访问，不接受任意 RAM 地址。
     *================================================================*/
    s16 SetOneValue2BothMem(u16 group, u16 index, u16 value)
    {
        u16 result = SstParams_Write(
            group, index, value); /* result：SstParams 返回的 SST_PARAM_* 状态，供参考接口转换成功/失败返回值。 */
        return result ? -(s16)result : 1;
    }
};
#endif
