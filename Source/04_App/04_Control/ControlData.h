/* 文件职责: 旧控制算法、PLL、PI 和参考历史量。数据只在所属 .c 定义，外部通过本头的必要声明访问。 */
#ifndef CONTROLDATA_H_
#define CONTROLDATA_H_
#include "SstControlConfig.h"
#include "SstTypes.h"
#include "SstControlConfig.h"
#include "ControlAlgorithmTypes.h"
#include "DpramRegister.h"

#ifdef __cplusplus
extern "C"
{
#endif

    extern unsigned char Flag_PC; /* Flag_PC：旧 PC/本地控制兼容标志。 */

    extern unsigned char Flag_DC_VoltSource; /* Flag_DC_VoltSource：原直流电压源控制模式标志。 */

    extern unsigned char Flag_DC_VoltSource_Set; /* Flag_DC_VoltSource_Set：原直流电压源模式设置完成标志。 */

    extern unsigned char flag_INV; /* flag_INV：原工频桥开环逆变模式标志。 */

    extern unsigned char flag_SVGOPEN; /* flag_SVGOPEN：原工频桥开环投入标志。 */

    extern unsigned char flag_DABOPEN; /* flag_DABOPEN：原 DAB 开环投入标志。 */

    extern unsigned char FLAG_RemRegulate_SetOver; /* FLAG_RemRegulate_SetOver：原遥调设置完成标志。 */

    extern unsigned char flag_SendDABRef_First; /* flag_SendDABRef_First：原 DAB 参考首次下发标志。 */

    extern unsigned char flag_SendSVGRef_First; /* flag_SendSVGRef_First：原工频桥参考首次下发标志。 */

    extern unsigned char Flag_Key; /* Flag_Key：原本地按键扫描状态。 */

    extern unsigned char Flag_SVG1s; /* Flag_SVG1s：Timer2 更新、后台消费的SVG 投入等待 1 s到时标志。 */

    extern float Sin_ph_Trans, /* Sin_ph_Trans：正弦坐标变换系数。 */
        Cos_ph_Trans, /* Cos_ph_Trans：余弦坐标变换系数。 */
        Sin_ph_Trans_NEW, /* Sin_ph_Trans_NEW：正弦坐标变换系数，输出变换使用值。 */
        Cos_ph_Trans_NEW; /* Cos_ph_Trans_NEW：余弦坐标变换系数，输出变换使用值。 */

    extern float Udc_Ref; /* Udc_Ref：旧直流电压控制参考，单位 V。 */

    extern float Current_Ref; /* Current_Ref：旧直流/交流电流控制参考，单位 A。 */

    extern float Ust_add; /* Ust_add：旧直流电压给定爬升增量，单位 V。 */

    extern float PLL_Factor; /* PLL_Factor：旧 PLL 系数调整因子。 */

    extern float Volt_Factor; /* Volt_Factor：旧总直流电压环系数调整因子。 */

    extern float PH_Volt_Factor; /* PH_Volt_Factor：旧相间电压平衡环系数调整因子。 */

    extern float Har_2_Factor; /* Har_2_Factor：2 次谐振控制系数调整因子。 */

    extern float Har_6_Factor; /* Har_6_Factor：6 次谐振控制系数调整因子。 */

    extern float Har_12_Factor; /* Har_12_Factor：12 次谐振控制系数调整因子。 */

    extern float Current_Factor; /* Current_Factor：旧正序电流环系数调整因子。 */

    extern float Neg_Current_Factor; /* Neg_Current_Factor：旧负序电流环系数调整因子。 */

    extern float AVolt_Factor; /* AVolt_Factor：旧交流电压相关调整因子。 */

    extern unsigned char TrigeSignal; /* TrigeSignal：原调试/录波触发信号。 */

    extern float Ref_ph_test; /* Ref_ph_test：原开环测试相角，单位 rad。 */

    extern float Ref_js_test; /* Ref_js_test：原调制测试幅值/计算缓存。 */

    extern float TEATabc[4]; /* TEATabc：原三相调试观察缓存，沿用原符号拼写。 */

#ifdef HVAC_online
    extern XXG_PID_Controller
        Neg_Current_d_PID_Controller, /* Neg_Current_d_PID_Controller：负序d 轴电流增量 PI 的系数与上一拍输入/输出历史状态。 */
        Neg_Current_q_PID_Controller; /* Neg_Current_q_PID_Controller：负序q 轴电流增量 PI 的系数与上一拍输入/输出历史状态。 */

#endif
#ifdef HVAC_online
    extern XXG_PID_Controller
        ClusterVolt_A_PID_Controller, /* ClusterVolt_A_PID_Controller：相间平衡直流电压_A增量 PI 的系数与上一拍输入/输出历史状态。 */
        ClusterVolt_B_PID_Controller, /* ClusterVolt_B_PID_Controller：相间平衡直流电压_B增量 PI 的系数与上一拍输入/输出历史状态。 */
        ClusterVolt_C_PID_Controller; /* ClusterVolt_C_PID_Controller：相间平衡直流电压_C增量 PI 的系数与上一拍输入/输出历史状态。 */

#endif
#ifdef HVAC_online
    extern XXG_H_Filter
        U_d_Grid_H2_20_Filter, /* U_d_Grid_H2_20_Filter：电网 d 轴电压_H2_20滤波器的四个历史输入/输出状态。 */
        U_d_Grid_H2_100_Filter, /* U_d_Grid_H2_100_Filter：电网 d 轴电压_H2_100滤波器的四个历史输入/输出状态。 */
        U_d_Grid_H6_100_Filter, /* U_d_Grid_H6_100_Filter：电网 d 轴电压_H6_100滤波器的四个历史输入/输出状态。 */
        U_d_Grid_H12_100_Filter; /* U_d_Grid_H12_100_Filter：电网 d 轴电压_H12_100滤波器的四个历史输入/输出状态。 */

#endif
#ifdef HVAC_online
    extern XXG_H_Filter
        U_q_Grid_H2_20_Filter, /* U_q_Grid_H2_20_Filter：电网 q 轴电压_H2_20滤波器的四个历史输入/输出状态。 */
        U_q_Grid_H2_100_Filter, /* U_q_Grid_H2_100_Filter：电网 q 轴电压_H2_100滤波器的四个历史输入/输出状态。 */
        U_q_Grid_H6_100_Filter, /* U_q_Grid_H6_100_Filter：电网 q 轴电压_H6_100滤波器的四个历史输入/输出状态。 */
        U_q_Grid_H12_100_Filter; /* U_q_Grid_H12_100_Filter：电网 q 轴电压_H12_100滤波器的四个历史输入/输出状态。 */

#endif
#ifdef HVAC_online
    extern XXG_H_Filter
        I_d_SVG_H2_20_Filter, /* I_d_SVG_H2_20_Filter：端口 d 轴电流_H2_20滤波器的四个历史输入/输出状态。 */
        I_d_SVG_H2_100_Filter, /* I_d_SVG_H2_100_Filter：端口 d 轴电流_H2_100滤波器的四个历史输入/输出状态。 */
        I_d_SVG_H6_100_Filter, /* I_d_SVG_H6_100_Filter：端口 d 轴电流_H6_100滤波器的四个历史输入/输出状态。 */
        I_d_SVG_H12_100_Filter; /* I_d_SVG_H12_100_Filter：端口 d 轴电流_H12_100滤波器的四个历史输入/输出状态。 */

#endif
#ifdef HVAC_online
    extern XXG_H_Filter
        I_q_SVG_H2_20_Filter, /* I_q_SVG_H2_20_Filter：端口 q 轴电流_H2_20滤波器的四个历史输入/输出状态。 */
        I_q_SVG_H2_100_Filter, /* I_q_SVG_H2_100_Filter：端口 q 轴电流_H2_100滤波器的四个历史输入/输出状态。 */
        I_q_SVG_H6_100_Filter, /* I_q_SVG_H6_100_Filter：端口 q 轴电流_H6_100滤波器的四个历史输入/输出状态。 */
        I_q_SVG_H12_100_Filter; /* I_q_SVG_H12_100_Filter：端口 q 轴电流_H12_100滤波器的四个历史输入/输出状态。 */

#endif
#ifdef HVAC_online
    extern XXG_H_Filter
        I_d_Load_H6_100_Filter, /* I_d_Load_H6_100_Filter：负载 d 轴电流_H6_100滤波器的四个历史输入/输出状态。 */
        I_d_Load_H12_100_Filter, /* I_d_Load_H12_100_Filter：负载 d 轴电流_H12_100滤波器的四个历史输入/输出状态。 */
        I_d_Load_H2_20_Filter; /* I_d_Load_H2_20_Filter：负载 d 轴电流_H2_20滤波器的四个历史输入/输出状态。 */

#endif
#ifdef HVAC_online
    extern XXG_H_Filter
        I_q_Load_H6_100_Filter, /* I_q_Load_H6_100_Filter：负载 q 轴电流_H6_100滤波器的四个历史输入/输出状态。 */
        I_q_Load_H12_100_Filter, /* I_q_Load_H12_100_Filter：负载 q 轴电流_H12_100滤波器的四个历史输入/输出状态。 */
        I_q_Load_H2_20_Filter; /* I_q_Load_H2_20_Filter：负载 q 轴电流_H2_20滤波器的四个历史输入/输出状态。 */

#endif
#ifdef HVAC_online
    extern XXG_H_Filter
        Va_Ave_H2_100_Filter, /* Va_Ave_H2_100_Filter：A 相直流平均电压_H2_100滤波器的四个历史输入/输出状态。 */
        Vb_Ave_H2_100_Filter, /* Vb_Ave_H2_100_Filter：B 相直流平均电压_H2_100滤波器的四个历史输入/输出状态。 */
        Vc_Ave_H2_100_Filter, /* Vc_Ave_H2_100_Filter：C 相直流平均电压_H2_100滤波器的四个历史输入/输出状态。 */
        Ave_H2_100_Filter, /* Ave_H2_100_Filter：平均量_H2_100滤波器的四个历史输入/输出状态。 */
        Ave_H6_100_Filter; /* Ave_H6_100_Filter：平均量_H6_100滤波器的四个历史输入/输出状态。 */

#endif
#ifdef HVAC_online
    extern XXG_H_Filter
        I_d_Ref_H2_100_Filter, /* I_d_Ref_H2_100_Filter：d 轴电流给定_H2_100滤波器的四个历史输入/输出状态。 */
        I_d_Ref_H6_100_Filter; /* I_d_Ref_H6_100_Filter：d 轴电流给定_H6_100滤波器的四个历史输入/输出状态。 */

#endif
#ifdef HVAC_online
    extern XXG_H_Filter P_H2_100_Filter, /* P_H2_100_Filter：P_H2_100滤波器的四个历史输入/输出状态。 */
        Q_H2_100_Filter, /* Q_H2_100_Filter：Q_H2_100滤波器的四个历史输入/输出状态。 */
        Q_Load_H2_100_Filter; /* Q_Load_H2_100_Filter：负载无功_H2_100滤波器的四个历史输入/输出状态。 */

#endif
#ifdef HVAC_online
    extern XXG_H_Filter H2_100_Filter, /* H2_100_Filter：通用 H2/100 滤波计算历史缓存。 */
        H6_100_Filter, /* H6_100_Filter：通用 H6/100 滤波计算历史缓存。 */
        H12_100_Filter, /* H12_100_Filter：通用 H12/100 滤波计算历史缓存。 */
        H2_20_Filter; /* H2_20_Filter：通用 H2/20 滤波计算历史缓存。 */

#endif
#ifdef HVAC_online
    extern XXG_Har_Controller I_d_2_Har_Controller, /* I_d_2_Har_Controller：I_d_2 次谐振控制器的历史输入/输出状态。 */
        I_q_2_Har_Controller; /* I_q_2_Har_Controller：I_q_2 次谐振控制器的历史输入/输出状态。 */

#endif
#ifdef HVAC_online
    extern XXG_Har_Controller I_d_6_Har_Controller, /* I_d_6_Har_Controller：I_d_6 次谐振控制器的历史输入/输出状态。 */
        I_q_6_Har_Controller; /* I_q_6_Har_Controller：I_q_6 次谐振控制器的历史输入/输出状态。 */

#endif
#ifdef HVAC_online
    extern XXG_Har_Controller
        I_d_12_Har_Controller, /* I_d_12_Har_Controller：I_d_12 次谐振控制器的历史输入/输出状态。 */
        I_q_12_Har_Controller; /* I_q_12_Har_Controller：I_q_12 次谐振控制器的历史输入/输出状态。 */

#endif
#ifdef HVAC_online
    extern XXG_Har_Controller Har_2_Controller, /* Har_2_Controller：2 次谐振通用历史输入/输出缓存。 */
        Har_6_Controller, /* Har_6_Controller：6 次谐振通用历史输入/输出缓存。 */
        Har_12_Controller; /* Har_12_Controller：12 次谐振通用历史输入/输出缓存。 */

#endif
#ifdef HVAC_online
    extern float Ugrid_d_Positive, /* Ugrid_d_Positive：电网电压 d 轴/分量的正序基波值，单位 V。 */
        Ugrid_d_Negative, /* Ugrid_d_Negative：电网电压 d 轴/分量的负序基波值，单位 V。 */
        Ugrid_hp_d; /* Ugrid_hp_d：电网电压 hp 轴/分量的d 轴谐波值，单位 V。 */

#endif
#ifdef HVAC_online
    extern float Ugrid_q_Positive, /* Ugrid_q_Positive：电网电压 q 轴/分量的正序基波值，单位 V。 */
        Ugrid_q_Negative, /* Ugrid_q_Negative：电网电压 q 轴/分量的负序基波值，单位 V。 */
        Ugrid_hp_q; /* Ugrid_hp_q：电网电压 hp 轴/分量的q 轴谐波值，单位 V。 */

#endif
#ifdef HVAC_online
    extern float Iin_d_Positive, /* Iin_d_Positive：端口电流 d 轴/分量的正序基波值，单位 A。 */
        Iin_d_Negative, /* Iin_d_Negative：端口电流 d 轴/分量的负序基波值，单位 A。 */
        Iin_hp_d; /* Iin_hp_d：端口电流 hp 轴/分量的d 轴谐波值，单位 A。 */

#endif
#ifdef HVAC_online
    extern float Iin_q_Positive, /* Iin_q_Positive：端口电流 q 轴/分量的正序基波值，单位 A。 */
        Iin_q_Negative, /* Iin_q_Negative：端口电流 q 轴/分量的负序基波值，单位 A。 */
        Iin_hp_q; /* Iin_hp_q：端口电流 hp 轴/分量的q 轴谐波值，单位 A。 */

#endif
#ifdef HVAC_online
    extern float Iload_d_Positive, /* Iload_d_Positive：负载电流 d 轴/分量的正序基波值，单位 A。 */
        Iload_d_Negative, /* Iload_d_Negative：负载电流 d 轴/分量的负序基波值，单位 A。 */
        Iload_har_d; /* Iload_har_d：负载电流 har 轴/分量的d 轴谐波值，单位 A。 */

#endif
#ifdef HVAC_online
    extern float Iload_q_Positive, /* Iload_q_Positive：负载电流 q 轴/分量的正序基波值，单位 A。 */
        Iload_q_Negative, /* Iload_q_Negative：负载电流 q 轴/分量的负序基波值，单位 A。 */
        Iload_har_q; /* Iload_har_q：负载电流 har 轴/分量的q 轴谐波值，单位 A。 */

#endif
#ifdef HVAC_online
    extern float Phase_PLL; /* Phase_PLL：旧 HVAC 锁相环电网相角，单位 rad。 */

#endif
#ifdef HVAC_online
    extern float Grid_P_Power, /* Grid_P_Power：旧电网有功功率计算值。 */
        Grid_Q_Power, /* Grid_Q_Power：旧电网无功功率计算值。 */
        Load_Q_Power, /* Load_Q_Power：旧负载无功功率计算值。 */
        SVG_Q_Power_Ref; /* SVG_Q_Power_Ref：旧 SVG 无功功率目标。 */

#endif
#ifdef HVAC_online
    extern float Id_Positive_Ref, /* Id_Positive_Ref：Id 正序电流给定，单位 A。 */
        Id_Negative_Ref, /* Id_Negative_Ref：Id 负序电流给定，单位 A。 */
        Id_Positive_Max, /* Id_Positive_Max：Id 正序电流给定上限，单位 A。 */
        Id_Positive_Min; /* Id_Positive_Min：Id 正序电流给定下限，单位 A。 */

#endif
#ifdef HVAC_online
    extern float Iq_Positive_Ref, /* Iq_Positive_Ref：Iq 正序电流给定，单位 A。 */
        Iq_Negative_Ref, /* Iq_Negative_Ref：Iq 负序电流给定，单位 A。 */
        Iq_Positive_Max, /* Iq_Positive_Max：Iq 正序电流给定上限，单位 A。 */
        Iq_Positive_Min; /* Iq_Positive_Min：Iq 正序电流给定下限，单位 A。 */

#endif
#ifdef HVAC_online
    extern float Uout_d_Positive, /* Uout_d_Positive：旧电流环 d_正序 输出电压分量/限幅值，单位 V。 */
        Uout_q_Positive, /* Uout_q_Positive：旧电流环 q_正序 输出电压分量/限幅值，单位 V。 */
        Uout_d_Negative, /* Uout_d_Negative：旧电流环 d_负序 输出电压分量/限幅值，单位 V。 */
        Uout_q_Negative, /* Uout_q_Negative：旧电流环 q_负序 输出电压分量/限幅值，单位 V。 */
        Uout_d_Positive_Max, /* Uout_d_Positive_Max：旧电流环 d_正序_上限 输出电压分量/限幅值，单位 V。 */
        Uout_d_Positive_Min, /* Uout_d_Positive_Min：旧电流环 d_正序_下限 输出电压分量/限幅值，单位 V。 */
        Uout_q_Positive_Max, /* Uout_q_Positive_Max：旧电流环 q_正序_上限 输出电压分量/限幅值，单位 V。 */
        Uout_q_Positive_Min; /* Uout_q_Positive_Min：旧电流环 q_正序_下限 输出电压分量/限幅值，单位 V。 */

#endif
#ifdef HVAC_online
    extern float Uout_A_Positive, /* Uout_A_Positive：旧电流环 A_正序 输出电压分量/限幅值，单位 V。 */
        Uout_B_Positive, /* Uout_B_Positive：旧电流环 B_正序 输出电压分量/限幅值，单位 V。 */
        Uout_C_Positive; /* Uout_C_Positive：旧电流环 C_正序 输出电压分量/限幅值，单位 V。 */

#endif
#ifdef HVAC_online
    extern float Har_5_PH, /* Har_5_PH：5 次谐波相位给定，单位 rad。 */
        Har_7_PH, /* Har_7_PH：7 次谐波相位给定，单位 rad。 */
        Har_11_PH; /* Har_11_PH：11 次谐波相位给定，单位 rad。 */

#endif
#ifdef HVAC_online
    extern float Har_5OR7_PH0, /* Har_5OR7_PH0：原 5OR7 次谐波初始相位，单位 rad。 */
        Har_11_PH0; /* Har_11_PH0：原 11 次谐波初始相位，单位 rad。 */

#endif
#ifdef HVAC_online
    extern float PLL_PID_Num_1; /* PLL_PID_Num_1：锁相环增量 PI 本拍误差系数。 */

#endif
#ifdef HVAC_online
    extern float PLL_PID_Num_0; /* PLL_PID_Num_0：锁相环增量 PI 上一拍误差系数。 */

#endif
#ifdef HVAC_online
    extern float Volt_PID_Num_1; /* Volt_PID_Num_1：电压增量 PI 本拍误差系数。 */

#endif
#ifdef HVAC_online
    extern float Volt_PID_Num_0; /* Volt_PID_Num_0：电压增量 PI 上一拍误差系数。 */

#endif
#ifdef HVAC_online
    extern float PH_Volt_PID_Num_1; /* PH_Volt_PID_Num_1：相间电压增量 PI 本拍误差系数。 */

#endif
#ifdef HVAC_online
    extern float PH_Volt_PID_Num_0; /* PH_Volt_PID_Num_0：相间电压增量 PI 上一拍误差系数。 */

#endif
#ifdef HVAC_online
    extern float Har_2_Controller_Num_2; /* Har_2_Controller_Num_2：2 次谐振控制器差分式分子第 2 项系数。 */

#endif
#ifdef HVAC_online
    extern float Har_2_Controller_Num_1; /* Har_2_Controller_Num_1：2 次谐振控制器差分式分子第 1 项系数。 */

#endif
#ifdef HVAC_online
    extern float Har_2_Controller_Num_0; /* Har_2_Controller_Num_0：2 次谐振控制器差分式分子第 0 项系数。 */

#endif
#ifdef HVAC_online
    extern float Har_2_Controller_Den_1; /* Har_2_Controller_Den_1：2 次谐振控制器差分式分母第 1 项系数。 */

#endif
#ifdef HVAC_online
    extern float Har_6_Controller_Num_2; /* Har_6_Controller_Num_2：6 次谐振控制器差分式分子第 2 项系数。 */

#endif
#ifdef HVAC_online
    extern float Har_6_Controller_Num_1; /* Har_6_Controller_Num_1：6 次谐振控制器差分式分子第 1 项系数。 */

#endif
#ifdef HVAC_online
    extern float Har_6_Controller_Num_0; /* Har_6_Controller_Num_0：6 次谐振控制器差分式分子第 0 项系数。 */

#endif
#ifdef HVAC_online
    extern float Har_6_Controller_Den_1; /* Har_6_Controller_Den_1：6 次谐振控制器差分式分母第 1 项系数。 */

#endif
#ifdef HVAC_online
    extern float Har_12_Controller_Num_2; /* Har_12_Controller_Num_2：12 次谐振控制器差分式分子第 2 项系数。 */

#endif
#ifdef HVAC_online
    extern float Har_12_Controller_Num_1; /* Har_12_Controller_Num_1：12 次谐振控制器差分式分子第 1 项系数。 */

#endif
#ifdef HVAC_online
    extern float Har_12_Controller_Num_0; /* Har_12_Controller_Num_0：12 次谐振控制器差分式分子第 0 项系数。 */

#endif
#ifdef HVAC_online
    extern float Har_12_Controller_Den_1; /* Har_12_Controller_Den_1：12 次谐振控制器差分式分母第 1 项系数。 */

#endif
#ifdef HVAC_online
    extern float Current_PID_Num_1; /* Current_PID_Num_1：电流增量 PI 本拍误差系数。 */

#endif
#ifdef HVAC_online
    extern float Current_PID_Num_0; /* Current_PID_Num_0：电流增量 PI 上一拍误差系数。 */

#endif
#ifdef HVAC_online
    extern float Current_PID_Num_1_Pres; /* Current_PID_Num_1_Pres：电流增量 PI 本拍误差系数的预设值。 */

#endif
#ifdef HVAC_online
    extern float Current_PID_Num_0_Pres; /* Current_PID_Num_0_Pres：电流增量 PI 上一拍误差系数的预设值。 */

#endif
#ifdef HVAC_online
    extern float Neg_Current_PID_Num_1; /* Neg_Current_PID_Num_1：负序电流增量 PI 本拍误差系数。 */

#endif
#ifdef HVAC_online
    extern float Neg_Current_PID_Num_0; /* Neg_Current_PID_Num_0：负序电流增量 PI 上一拍误差系数。 */

#endif
#ifdef HVAC_online
    extern float VA_Control, /* VA_Control：A 相原输出电压控制给定，单位 V。 */
        VB_Control, /* VB_Control：B 相原输出电压控制给定，单位 V。 */
        VC_Control; /* VC_Control：C 相原输出电压控制给定，单位 V。 */

#endif
#ifdef HVAC_online
    extern float
        Iin_d_Positive_last, /* Iin_d_Positive_last：端口电流 d 轴/分量的正序基波值，单位 A；上一拍噪声比较缓存。 */
        Iin_q_Positive_last; /* Iin_q_Positive_last：端口电流 q 轴/分量的正序基波值，单位 A；上一拍噪声比较缓存。 */

#endif
#ifdef HVAC_online
    extern ushort GNDSwitch, /* GNDSwitch：原接地开关状态。 */
        ISOSwitch; /* ISOSwitch：原隔离开关状态。 */

#endif
#ifdef HVAC_online
    extern ushort SM_write_flag; /* SM_write_flag：旧维护存储写入状态标志。 */

#endif
#ifdef HVAC_online
    extern ushort SM_write_count; /* SM_write_count：旧维护存储写入的累计计数。 */

#endif
#ifdef HVAC_online
    extern ushort ISO_counter_on, /* ISO_counter_on：原隔离开关闭合确认计数。 */
        ISO_counter_off; /* ISO_counter_off：原隔离开关断开确认计数。 */

#endif
#ifdef HVAC_online
    extern ushort HA_DC_Log_A; /* HA_DC_Log_A：高压交流端口 A 相/相间直流电压排序方向字。 */

#endif
#ifdef HVAC_online
    extern ushort HA_DC_Log_B; /* HA_DC_Log_B：高压交流端口 B 相/相间直流电压排序方向字。 */

#endif
#ifdef HVAC_online
    extern ushort HA_DC_Log_C; /* HA_DC_Log_C：高压交流端口 C 相/相间直流电压排序方向字。 */

#endif
#ifdef HVAC_online
    extern ushort HA_IG_Log_A; /* HA_IG_Log_A：高压交流端口 A 相/相间电流方向控制字。 */

#endif
#ifdef HVAC_online
    extern ushort HA_IG_Log_B; /* HA_IG_Log_B：高压交流端口 B 相/相间电流方向控制字。 */

#endif
#ifdef HVAC_online
    extern ushort HA_IG_Log_C; /* HA_IG_Log_C：高压交流端口 C 相/相间电流方向控制字。 */

#endif
#ifdef HVAC_online
    extern float
        HA_ReferenceDEF_A, /* HA_ReferenceDEF_A：高压交流端口 A 相/相间按在线模块数得到的 FPGA 调制参考基值。 */
        HA_ReferenceDEF_B, /* HA_ReferenceDEF_B：高压交流端口 B 相/相间按在线模块数得到的 FPGA 调制参考基值。 */
        HA_ReferenceDEF_C; /* HA_ReferenceDEF_C：高压交流端口 C 相/相间按在线模块数得到的 FPGA 调制参考基值。 */

#endif
#ifdef HVAC_online
    extern ushort NHA_module_A; /* NHA_module_A：HA 端口 A 相在线模块数量，用于采样平均及调制尺度。 */

#endif
#ifdef HVAC_online
    extern ushort NHA_module_B; /* NHA_module_B：HA 端口 B 相在线模块数量，用于采样平均及调制尺度。 */

#endif
#ifdef HVAC_online
    extern ushort NHA_module_C; /* NHA_module_C：HA 端口 C 相在线模块数量，用于采样平均及调制尺度。 */

#endif
#ifdef HVAC_online
    extern ushort HA_PM_A1; /* HA_PM_A1：HA 端口 A1 组在线模块位图，每位对应一个模块。 */

#endif
#ifdef HVAC_online
    extern ushort HA_PM_A2; /* HA_PM_A2：HA 端口 A2 组在线模块位图，每位对应一个模块。 */

#endif
#ifdef HVAC_online
    extern ushort HA_PM_B1; /* HA_PM_B1：HA 端口 B1 组在线模块位图，每位对应一个模块。 */

#endif
#ifdef HVAC_online
    extern ushort HA_PM_B2; /* HA_PM_B2：HA 端口 B2 组在线模块位图，每位对应一个模块。 */

#endif
#ifdef HVAC_online
    extern ushort HA_PM_C1; /* HA_PM_C1：HA 端口 C1 组在线模块位图，每位对应一个模块。 */

#endif
#ifdef HVAC_online
    extern ushort HA_PM_C2; /* HA_PM_C2：HA 端口 C2 组在线模块位图，每位对应一个模块。 */

#endif
#ifdef HVAC_online
    extern float Ust_HVAC_Port; /* Ust_HVAC_Port：旧 HVAC 单模块直流电压运行给定，单位 V。 */

#endif
#ifdef HVAC_online
    extern float UstOK_HVAC_Port; /* UstOK_HVAC_Port：旧 HVAC 启动电压达标值，单位 V。 */

#endif
#ifdef HVAC_online
    extern unsigned char Flag_SlopeTime; /* Flag_SlopeTime：旧电压给定斜坡计时使能。 */

#endif
#ifdef HVAC_online
    extern ushort SlopeTime_N; /* SlopeTime_N：旧电压斜坡的计时长度/计数。 */

#endif
#ifdef HVAC_online
    extern float HA_Module_RefUdc; /* HA_Module_RefUdc：旧 HVAC 单模块直流电压目标，单位 V。 */

#endif
#ifdef HVAC_online
    extern float Ud_st_Max; /* Ud_st_Max：Ud 原控制给定上限，电压 V、电流 A。 */

#endif
#ifdef HVAC_online
    extern float Ud_st_Min; /* Ud_st_Min：Ud 原控制给定下限，电压 V、电流 A。 */

#endif
#ifdef HVAC_online
    extern float Uq_st_Max; /* Uq_st_Max：Uq 原控制给定上限，电压 V、电流 A。 */

#endif
#ifdef HVAC_online
    extern float Uq_st_Min; /* Uq_st_Min：Uq 原控制给定下限，电压 V、电流 A。 */

#endif
#ifdef HVAC_online
    extern float Idst_Max; /* Idst_Max：Idst 原控制给定上限，电压 V、电流 A。 */

#endif
#ifdef HVAC_online
    extern float Idst_Min; /* Idst_Min：Idst 原控制给定下限，电压 V、电流 A。 */

#endif
#ifdef HVAC_online
    extern float Iqst_Max; /* Iqst_Max：Iqst 原控制给定上限，电压 V、电流 A。 */

#endif
#ifdef HVAC_online
    extern float Iqst_Min; /* Iqst_Min：Iqst 原控制给定下限，电压 V、电流 A。 */

#endif
    extern float Ust_add; /* Ust_add：旧直流电压给定爬升增量，单位 V。 */

#ifdef HVAC_online
    extern float Phase_PLL_First; /* Phase_PLL_First：旧 PLL 首拍相角缓存，单位 rad。 */

#endif
#ifdef HVAC_online
    extern float PLL_Phase_Com; /* PLL_Phase_Com：旧电流变换相位补偿，单位 rad。 */

#endif
#ifdef HVAC_online
    extern float PLL_Phase_Com_NEW; /* PLL_Phase_Com_NEW：旧输出变换相位补偿，单位 rad。 */

#endif
#ifdef HVAC_online
    extern float PhaseCom; /* PhaseCom：旧附加相位补偿量，单位 rad。 */

#endif
#ifdef HVAC_online
    extern float Sin_ph_Trans_Com; /* Sin_ph_Trans_Com：正弦坐标变换系数，含原相位补偿。 */

#endif
#ifdef HVAC_online
    extern float Cos_ph_Trans_Com; /* Cos_ph_Trans_Com：余弦坐标变换系数，含原相位补偿。 */

#endif
#ifdef HVAC_online
    extern float Sin_ph_Trans_Com_NEW; /* Sin_ph_Trans_Com_NEW：正弦坐标变换系数，含原相位补偿，输出变换使用值。 */

#endif
#ifdef HVAC_online
    extern float Cos_ph_Trans_Com_NEW; /* Cos_ph_Trans_Com_NEW：余弦坐标变换系数，含原相位补偿，输出变换使用值。 */

#endif
#ifdef HVAC_online
    extern float Sin_ph_Trans_next; /* Sin_ph_Trans_next：正弦坐标变换系数，下一拍预测值。 */

#endif
#ifdef HVAC_online
    extern float Cos_ph_Trans_next; /* Cos_ph_Trans_next：余弦坐标变换系数，下一拍预测值。 */

#endif
#ifdef HVAC_online
    extern float Ugrid_Max; /* Ugrid_Max：旧控制使用的电网电压幅值上限/估计值，单位 V。 */

#endif
#ifdef HVAC_online
    extern float HA_Module_RefVolt_A; /* HA_Module_RefVolt_A：高压交流端口 A 相/相间模块电压调制给定，单位 V。 */

#endif
#ifdef HVAC_online
    extern float HA_Module_RefVolt_B; /* HA_Module_RefVolt_B：高压交流端口 B 相/相间模块电压调制给定，单位 V。 */

#endif
#ifdef HVAC_online
    extern float HA_Module_RefVolt_C; /* HA_Module_RefVolt_C：高压交流端口 C 相/相间模块电压调制给定，单位 V。 */

#endif
#ifdef HVAC_online
    extern ushort HA_DABDuty_A; /* HA_DABDuty_A：高压交流端口 A 相/相间DAB 占空比 FPGA 计数。 */

#endif
#ifdef HVAC_online
    extern ushort HA_DABDuty_B; /* HA_DABDuty_B：高压交流端口 B 相/相间DAB 占空比 FPGA 计数。 */

#endif
#ifdef HVAC_online
    extern ushort HA_DABDuty_C; /* HA_DABDuty_C：高压交流端口 C 相/相间DAB 占空比 FPGA 计数。 */

#endif
#ifdef HVAC_online
    extern ushort HA_DAB_PluseStart_A; /* HA_DAB_PluseStart_A：高压交流端口 A 相/相间DAB 脉冲起始 FPGA 计数。 */

#endif
#ifdef HVAC_online
    extern ushort HA_DAB_PluseStart_B; /* HA_DAB_PluseStart_B：高压交流端口 B 相/相间DAB 脉冲起始 FPGA 计数。 */

#endif
#ifdef HVAC_online
    extern ushort HA_DAB_PluseStart_C; /* HA_DAB_PluseStart_C：高压交流端口 C 相/相间DAB 脉冲起始 FPGA 计数。 */

#endif
#ifdef HVAC_online
    extern ushort HA_DAB_T; /* HA_DAB_T：高压交流端口 DAB 周期 FPGA 计数。 */

#endif
#ifdef HVAC_online
    extern unsigned char Add_3th; /* Add_3th：旧三次谐波注入使能标志。 */

#endif
#ifdef HVAC_online
    extern unsigned char Method_3th; /* Method_3th：旧三次谐波注入算法选择。 */

#endif
#ifdef HVAC_online
    extern unsigned char flag_Standard; /* flag_Standard：旧调制标准化方式选择标志。 */

#endif
#ifdef HVAC_online
    extern float StandardValue; /* StandardValue：旧调制参考标准化基值。 */

#endif
#ifdef HVAC_online
    extern unsigned char DAB_ONLY; /* DAB_ONLY：旧仅运行 DAB 的测试选择标志。 */

#endif
#ifdef __cplusplus
}
#endif
#endif
