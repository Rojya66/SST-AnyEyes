#include "SstControlConfig.h"
/* 文件职责: DpramDefaults 所属数据域的默认初始化；保持原连续语句段。 */
#include "DpramDefaults.h"
#include "DpramHostData.h"

#ifdef HVAC_online
/*==================================================================
 * 函数功能: 建立原DPRAM遥调缓存的默认保护使能和给定
 * 输入参数: 无
 * 输出参数: 更新所属数据域及原硬件寄存器；语句顺序保持原初始化段。
 * 返回值: 无
 * 执行说明: 仅开机调用；不同分域按ControlInit列出的原顺序执行。
 *================================================================*/
void DpramDefaults_InitRegulation(void)
{
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.COM_SYS_MODE.Bbits.HA_RunMode = 1; //运行
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.COM_SYS_MODE.Bbits.HA_CtlMode = 0; //手动

    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.AD_PROT_EN.AD_PORT_EN16 = 0x7000;

    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_1_EN.FO_EN16 = 0x3F7F;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_2_EN.FO_EN16 = 0x3F7F;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_3_EN.FO_EN16 = 0x3F7F;

    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FB_EN.FB_EN16 = 0x4441;

    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_P_SET = 0;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_Q_SET = 0;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET = 680;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F_SET = 500;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.RMS_CURRENT_5TH_SET = 0;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.RMS_CURRENT_7TH_SET = 0;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.RMS_CURRENT_11TH_SET = 0;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PHA_CURRENT_5TH_SET = 0;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PHA_CURRENT_7TH_SET = 0;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PHA_CURRENT_11TH_SET = 0;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_V_SET = 0;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_PF_SET = 100;
    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_Q_SET = 0;
}

#endif /* HVAC_online */
