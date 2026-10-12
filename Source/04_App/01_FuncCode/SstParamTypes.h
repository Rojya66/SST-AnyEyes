#ifndef SST_PARAM_TYPES_H
#define SST_PARAM_TYPES_H
#include <stdint.h>
/* SST_PARAM_GROUPS：参数服务固定参数组数，参考协议为 30。 */
#define SST_PARAM_GROUPS 30U
/* SST_PARAM_PER_GROUP：参数服务每组固定参数字数，参考协议为 34。 */
#define SST_PARAM_PER_GROUP 34U
/* SST_PARAM_OK：参数服务成功结果码。 */
#define SST_PARAM_OK 0U
/* SST_PARAM_BAD_ADDRESS：参数服务地址非法结果码。 */
#define SST_PARAM_BAD_ADDRESS 1U
/* SST_PARAM_READ_ONLY：参数服务只读地址拒绝结果码。 */
#define SST_PARAM_READ_ONLY 2U
/* SST_PARAM_RANGE：参数服务数值越界结果码。 */
#define SST_PARAM_RANGE 3U
/* SST_PARAM_BUSY：参数服务忙或不满足停机条件结果码。 */
#define SST_PARAM_BUSY 4U
/* SST_PARAM_DENIED：参数服务编译权限拒绝结果码。 */
#define SST_PARAM_DENIED 5U
/* SST_PARAM_STORAGE：参数服务存储校验/读写失败结果码。 */
#define SST_PARAM_STORAGE 6U

#endif /* SST_PARAM_TYPES_H */
