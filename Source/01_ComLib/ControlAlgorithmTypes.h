/* 文件职责: 模块接口。数据只在所属 .c 定义，外部通过本头的必要声明访问。 */
#ifndef CONTROLALGORITHMTYPES_H_
#define CONTROLALGORITHMTYPES_H_
#include "SstTypes.h"

#ifdef __cplusplus
extern "C"
{
#endif
    typedef union _XXG_H_Filter
    {
        struct _H_Filter
        {
            float IN_n_1; /* IN_n_1：前一拍输入历史值，单位与该控制器输入/输出一致。 */
            float IN_n_2; /* IN_n_2：前两拍输入历史值，单位与该控制器输入/输出一致。 */
            float OUT_n_1; /* OUT_n_1：前一拍输出历史值，单位与该控制器输入/输出一致。 */
            float OUT_n_2; /* OUT_n_2：前两拍输出历史值，单位与该控制器输入/输出一致。 */
        } H_Filter; /* H_Filter：对应结构化字段视图，与本联合体的整字数组重叠，布局保持原工程。 */

        float H_Filter_BUF[4]; /* H_Filter_BUF：本联合体的连续原始字/状态数组视图，供复制或存储打包使用。 */
    } XXG_H_Filter;

    typedef union _XXG_Har_Controller
    {
        struct _Har_Controller
        {
            float IN_n_1; /* IN_n_1：前一拍输入历史值，单位与该控制器输入/输出一致。 */
            float IN_n_2; /* IN_n_2：前两拍输入历史值，单位与该控制器输入/输出一致。 */
            float OUT_n_1; /* OUT_n_1：前一拍输出历史值，单位与该控制器输入/输出一致。 */
            float OUT_n_2; /* OUT_n_2：前两拍输出历史值，单位与该控制器输入/输出一致。 */
        } Har_Controller; /* Har_Controller：对应结构化字段视图，与本联合体的整字数组重叠，布局保持原工程。 */

        float Har_Controller_BUF[4]; /* Har_Controller_BUF：本联合体的连续原始字/状态数组视图，供复制或存储打包使用。 */
    } XXG_Har_Controller;

    typedef union _XXG_PID_Controller
    {
        struct _PID_Controller
        {
            float Num_1; /* Num_1：增量 PI 的本拍误差系数。 */
            float Num_0; /* Num_0：增量 PI 的前一拍误差系数。 */
            float IN_n_1; /* IN_n_1：前一拍输入历史值，单位与该控制器输入/输出一致。 */
            float OUT_n_1; /* OUT_n_1：前一拍输出历史值，单位与该控制器输入/输出一致。 */
        } PID_Controller; /* PID_Controller：对应结构化字段视图，与本联合体的整字数组重叠，布局保持原工程。 */

        float PID_Controller_BUF[4]; /* PID_Controller_BUF：本联合体的连续原始字/状态数组视图，供复制或存储打包使用。 */
    } XXG_PID_Controller;

#ifdef __cplusplus
}
#endif
#endif
