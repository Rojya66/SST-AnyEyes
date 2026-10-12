#include "ControlData.h"
#include "DpramHostData.h"
#include "PortStatusData.h"
#include "SstControlConfig.h"
#include "SstTypes.h"
// Tsinghua University PERET File
// Checkin Date: 2018.09.18
//###########################################################################
//
// FILE:   X1226_RW.c
//
// TITLE:  DSP data read from or write to X1226
// AUTHOR: Li Kai
//
//###########################################################################
// Tsinghua University PERET Release: DSP_C6748 Function Files V1.0
// Release Date: 2018.09.18
//###########################################################################

#include "EEProm.h"
#include "X1226_RW.H"
/****************************************************************************/
/*                                                                          */
/*              函数声明                                                                                                                                                      */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 读取旧版 EEPROM 第 0 页并恢复对应端口参数
 * 输入参数: 无
 * 输出参数: 参数全局量或指定 EEPROM 页。
 * 返回值: 无
 * 执行说明: 旧版存储兼容函数；页映射保持原项目定义。
 *================================================================*/
void EEPROM_Page0_2_DSP(void);
/*==================================================================
 * 函数功能: 读取旧版 EEPROM 第 1 页并恢复对应端口参数
 * 输入参数: 无
 * 输出参数: 参数全局量或指定 EEPROM 页。
 * 返回值: 无
 * 执行说明: 旧版存储兼容函数；页映射保持原项目定义。
 *================================================================*/
void EEPROM_Page1_2_DSP(void);
/*==================================================================
 * 函数功能: 读取旧版 EEPROM 第 2 页并恢复对应端口参数
 * 输入参数: 无
 * 输出参数: 参数全局量或指定 EEPROM 页。
 * 返回值: 无
 * 执行说明: 旧版存储兼容函数；页映射保持原项目定义。
 *================================================================*/
void EEPROM_Page2_2_DSP(void);
/*==================================================================
 * 函数功能: 读取旧版 EEPROM 第 3 页并恢复对应端口参数
 * 输入参数: 无
 * 输出参数: 参数全局量或指定 EEPROM 页。
 * 返回值: 无
 * 执行说明: 旧版存储兼容函数；页映射保持原项目定义。
 *================================================================*/
void EEPROM_Page3_2_DSP(void);
/*==================================================================
 * 函数功能: 把对应端口旧版参数打包写入 EEPROM 第 0 页
 * 输入参数: 无
 * 输出参数: 参数全局量或指定 EEPROM 页。
 * 返回值: 无
 * 执行说明: 旧版存储兼容函数；页映射保持原项目定义。
 *================================================================*/
void DSP_2_EEPROM_Page0(void);
/*==================================================================
 * 函数功能: 把对应端口旧版参数打包写入 EEPROM 第 1 页
 * 输入参数: 无
 * 输出参数: 参数全局量或指定 EEPROM 页。
 * 返回值: 无
 * 执行说明: 旧版存储兼容函数；页映射保持原项目定义。
 *================================================================*/
void DSP_2_EEPROM_Page1(void);
/*==================================================================
 * 函数功能: 把对应端口旧版参数打包写入 EEPROM 第 2 页
 * 输入参数: 无
 * 输出参数: 参数全局量或指定 EEPROM 页。
 * 返回值: 无
 * 执行说明: 旧版存储兼容函数；页映射保持原项目定义。
 *================================================================*/
void DSP_2_EEPROM_Page2(void);
/*==================================================================
 * 函数功能: 把对应端口旧版参数打包写入 EEPROM 第 3 页
 * 输入参数: 无
 * 输出参数: 参数全局量或指定 EEPROM 页。
 * 返回值: 无
 * 执行说明: 旧版存储兼容函数；页映射保持原项目定义。
 *================================================================*/
void DSP_2_EEPROM_Page3(void);
/****************************************************************************/
/*                                                                          */
/*              将EEPROM的第0页数据写入DSP                                  */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 读取旧版 EEPROM 第 0 页并恢复对应端口参数
 * 输入参数: 无
 * 输出参数: 参数全局量或指定 EEPROM 页。
 * 返回值: 无
 * 执行说明: 旧版存储兼容函数；页映射保持原项目定义。
 *================================================================*/
void EEPROM_Page0_2_DSP(void)
{
	Uint8 DataTemp[4]; /* DataTemp：旧 EEPROM 页的原始字节打包/解包联合缓存。 */
	r_eeprom_section_read(0, 8, 4, DataTemp);
	switch(DataTemp[0])
	  {
	  	case 0:  {Port_Main_WorkMode=0;break;}
	  	case 1:  {Port_Main_WorkMode=1;break;}
	  	case 2:  {Port_Main_WorkMode=2;break;}
	  	case 3:  {Port_Main_WorkMode=3;break;}
	  	default: {break;}
		}
	switch(DataTemp[1])
	  {
	  	case 0:  {Port_STATCOM_WorkMode=0;break;}
	  	case 1:  {Port_STATCOM_WorkMode=1;break;}
	  	case 2:  {Port_STATCOM_WorkMode=2;break;}
	  	case 3:  {Port_STATCOM_WorkMode=3;break;}
	  	default: {break;}
		}
	switch(DataTemp[2])
	  {
	  	case 0:  {Port_APF_WorkMode=0;break;}
	  	case 1:  {Port_APF_WorkMode=1;break;}
	  	case 2:  {Port_APF_WorkMode=2;break;}
	  	default: {break;}
		}
	switch(DataTemp[3])
	  {
	  	case 0:  {SW.Bbits.S_EXT_CB_ON_EN=0;break;}
	  	case 1:  {SW.Bbits.S_EXT_CB_ON_EN=1;break;}
	  	default: {break;}
		}			
}

/****************************************************************************/
/*                                                                          */
/*              将EEPROM的第1页数据写入DSP                                  */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 读取旧版 EEPROM 第 1 页并恢复对应端口参数
 * 输入参数: 无
 * 输出参数: 参数全局量或指定 EEPROM 页。
 * 返回值: 无
 * 执行说明: 旧版存储兼容函数；页映射保持原项目定义。
 *================================================================*/
void EEPROM_Page1_2_DSP(void)
{
	Uint8 DataTemp[54]; /* DataTemp：旧 EEPROM 页的原始字节打包/解包联合缓存。 */
	ushort EE_TEMP; /* EE_TEMP：旧 EEPROM 浮点参数的两字联合换算缓存。 */
	Uint8 i; /* i：本次数组、参数或模块遍历的索引。 */
	r_eeprom_section_read(1, 0, 54, DataTemp);
	
	EE_TEMP=DataTemp[0];	
	
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.COM_SYS_MODE.COM_SYS_MODE16=(EE_TEMP<<8);
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.COM_SYS_MODE.COM_SYS_MODE16+=DataTemp[1];
	
	EE_TEMP=DataTemp[2];
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.AD_PROT_EN.AD_PORT_EN16=(EE_TEMP<<8);
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.AD_PROT_EN.AD_PORT_EN16+=DataTemp[3];
	
	EE_TEMP=DataTemp[4];
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_1_EN.FO_EN16=(EE_TEMP<<8);
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_1_EN.FO_EN16+=DataTemp[5];
	
	EE_TEMP=DataTemp[6];
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_2_EN.FO_EN16=(EE_TEMP<<8);
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_2_EN.FO_EN16+=DataTemp[7];
	
	EE_TEMP=DataTemp[8];
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_3_EN.FO_EN16=(EE_TEMP<<8);
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_3_EN.FO_EN16+=DataTemp[9];
	
	EE_TEMP=DataTemp[10];
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FB_EN.FB_EN16=(EE_TEMP<<8);
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FB_EN.FB_EN16+=DataTemp[11];
	
	EE_TEMP=DataTemp[12];
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_P_SET=(EE_TEMP<<8);
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_P_SET+=DataTemp[13];
	
	EE_TEMP=DataTemp[14];
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_Q_SET=(EE_TEMP<<8);
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_Q_SET+=DataTemp[15];
	
	EE_TEMP=DataTemp[16];
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET=(EE_TEMP<<8);
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET+=DataTemp[17];
	
	EE_TEMP=DataTemp[18];
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F_SET=(EE_TEMP<<8);
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F_SET+=DataTemp[19];
	
	EE_TEMP=DataTemp[20];
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.RMS_CURRENT_5TH_SET=(EE_TEMP<<8);
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.RMS_CURRENT_5TH_SET+=DataTemp[21];
	
	EE_TEMP=DataTemp[22];
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.RMS_CURRENT_7TH_SET=(EE_TEMP<<8);
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.RMS_CURRENT_7TH_SET+=DataTemp[23];
	
	EE_TEMP=DataTemp[24];
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.RMS_CURRENT_11TH_SET=(EE_TEMP<<8);
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.RMS_CURRENT_11TH_SET+=DataTemp[25];
	
	EE_TEMP=DataTemp[26];
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PHA_CURRENT_5TH_SET=(EE_TEMP<<8);
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PHA_CURRENT_5TH_SET+=DataTemp[27];
	
	EE_TEMP=DataTemp[28];
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PHA_CURRENT_7TH_SET=(EE_TEMP<<8);
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PHA_CURRENT_7TH_SET+=DataTemp[29];
	
	EE_TEMP=DataTemp[30];
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PHA_CURRENT_11TH_SET=(EE_TEMP<<8);
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PHA_CURRENT_11TH_SET+=DataTemp[31];
	
	EE_TEMP=DataTemp[32];
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_V_SET=(EE_TEMP<<8);
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_V_SET+=DataTemp[33];
	
	EE_TEMP=DataTemp[34];
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_PF_SET=(EE_TEMP<<8);
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_PF_SET+=DataTemp[35];
	
	EE_TEMP=DataTemp[36];
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_Q_SET=(EE_TEMP<<8);
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_Q_SET+=DataTemp[37];
	
	EE_TEMP=DataTemp[38];
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V0_SET=(EE_TEMP<<8);
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V0_SET+=DataTemp[39];
	
	EE_TEMP=DataTemp[40];
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F0_SET=(EE_TEMP<<8);
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F0_SET+=DataTemp[41];
	
	EE_TEMP=DataTemp[42];
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KF_SET=(EE_TEMP<<8);
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KF_SET+=DataTemp[43];
	
	EE_TEMP=DataTemp[44];
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KV_SET=(EE_TEMP<<8);
	PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KV_SET+=DataTemp[45];
	
	for (i=0;i<4;i++)
	{
		EE_TEMP=DataTemp[46+i*2];
		PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[i]=(EE_TEMP<<8);
		PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[i]+=DataTemp[47+i*2];
	}
}

/****************************************************************************/
/*                                                                          */
/*              将EEPROM的第2页数据写入DSP                                  */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 读取旧版 EEPROM 第 2 页并恢复对应端口参数
 * 输入参数: 无
 * 输出参数: 参数全局量或指定 EEPROM 页。
 * 返回值: 无
 * 执行说明: 旧版存储兼容函数；页映射保持原项目定义。
 *================================================================*/
void EEPROM_Page2_2_DSP(void)
{
	ushort EE_TEMP; /* EE_TEMP：旧 EEPROM 浮点参数的两字联合换算缓存。 */
  /************************PI参数************************/
  Uint8 DataTemp[24]; /* DataTemp：旧 EEPROM 页的原始字节打包/解包联合缓存。 */
//	Uint8 i;
	r_eeprom_section_read(2, 0, 24, DataTemp);
	
	EE_TEMP=DataTemp[0];	
  PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_1=(EE_TEMP<<8);
  PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_1+=DataTemp[1];
  
  EE_TEMP=DataTemp[2];	
  PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1=(EE_TEMP<<8);
  PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1+=DataTemp[3];
  
  EE_TEMP=DataTemp[4];	
  PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1=(EE_TEMP<<8);
  PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1+=DataTemp[5];
  
  EE_TEMP=DataTemp[6];	
  PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_1=(EE_TEMP<<8);
  PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_1+=DataTemp[7];
  
  EE_TEMP=DataTemp[8];	
  PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_2=(EE_TEMP<<8);
  PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_2+=DataTemp[9];
  
  EE_TEMP=DataTemp[10];	
  PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_2=(EE_TEMP<<8);
  PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_2+=DataTemp[11];
  
  EE_TEMP=DataTemp[12];	
  PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_2=(EE_TEMP<<8);
  PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_2+=DataTemp[13];
  
  EE_TEMP=DataTemp[14];	
  PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_2=(EE_TEMP<<8);
  PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_2+=DataTemp[15];
  
  EE_TEMP=DataTemp[16];	
  PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_3=(EE_TEMP<<8);
  PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_3+=DataTemp[17];
  
  EE_TEMP=DataTemp[18];	
  PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_3=(EE_TEMP<<8);
  PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_3+=DataTemp[19];
  
  EE_TEMP=DataTemp[20];	
  PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_3=(EE_TEMP<<8);
  PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_3+=DataTemp[21];
  
  EE_TEMP=DataTemp[22];	
  PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_3=(EE_TEMP<<8);
  PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_3+=DataTemp[23];

}

/****************************************************************************/
/*                                                                          */
/*              将EEPROM的第3页数据写入DSP                                  */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 读取旧版 EEPROM 第 3 页并恢复对应端口参数
 * 输入参数: 无
 * 输出参数: 参数全局量或指定 EEPROM 页。
 * 返回值: 无
 * 执行说明: 旧版存储兼容函数；页映射保持原项目定义。
 *================================================================*/
void EEPROM_Page3_2_DSP(void)
{
	//读EEPROM的第3页的遥调量	/************************EER2的数据************************/
	ushort EE_TEMP; /* EE_TEMP：旧 EEPROM 浮点参数的两字联合换算缓存。 */
	Uint8 DataTemp[8]; /* DataTemp：旧 EEPROM 页的原始字节打包/解包联合缓存。 */
	Uint8 i; /* i：本次数组、参数或模块遍历的索引。 */
	r_eeprom_section_read(3, 0, 8, DataTemp);
	
	for (i=0;i<4;i++) 
	{
		EE_TEMP=DataTemp[2*i];	
		PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.EER2_DATA[i]=(EE_TEMP<<8);
		PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.EER2_DATA[i]+=DataTemp[2*i+1];
	}
}


/****************************************************************************/
/*                                                                          */
/*              将DSP的数据写入EEPROM的第0页                                */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 把对应端口旧版参数打包写入 EEPROM 第 0 页
 * 输入参数: 无
 * 输出参数: 参数全局量或指定 EEPROM 页。
 * 返回值: 无
 * 执行说明: 旧版存储兼容函数；页映射保持原项目定义。
 *================================================================*/
void DSP_2_EEPROM_Page0(void)
{
	Uint8 DataTemp[4]; /* DataTemp：旧 EEPROM 页的原始字节打包/解包联合缓存。 */
	switch(Port_Main_WorkMode)
	  {
	  	case 0:  {DataTemp[0]=0;break;}
	  	case 1:  {DataTemp[0]=1;break;}
	  	case 2:  {DataTemp[0]=2;break;}
	  	case 3:  {DataTemp[0]=3;break;}
	  	default: {break;}
		}
	switch(Port_STATCOM_WorkMode)
	  {
	  	case 0:  {DataTemp[1]=0;break;}
	  	case 1:  {DataTemp[1]=1;break;}
	  	case 2:  {DataTemp[1]=2;break;}
	  	case 3:  {DataTemp[1]=3;break;}
	  	default: {break;}
		}
	switch(Port_APF_WorkMode)
	  {
	  	case 0:  {DataTemp[2]=0;break;}
	  	case 1:  {DataTemp[2]=1;break;}
	  	case 2:  {DataTemp[2]=2;break;}
	  	default: {break;}
		}
	switch(SW.Bbits.S_EXT_CB_ON_EN)
	  {
	  	case 0:  {DataTemp[3]=0;break;}
	  	case 1:  {DataTemp[3]=1;break;}
	  	default: {break;}
		}
	r_eeprom_section_write(0, 8, 4, DataTemp);			
}

/****************************************************************************/
/*                                                                          */
/*              将DSP的数据写入EEPROM的第1页                                */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 把对应端口旧版参数打包写入 EEPROM 第 1 页
 * 输入参数: 无
 * 输出参数: 参数全局量或指定 EEPROM 页。
 * 返回值: 无
 * 执行说明: 旧版存储兼容函数；页映射保持原项目定义。
 *================================================================*/
void DSP_2_EEPROM_Page1(void)
{
	Uint8 DataTemp[54]; /* DataTemp：旧 EEPROM 页的原始字节打包/解包联合缓存。 */
	Uint8 i; /* i：本次数组、参数或模块遍历的索引。 */
	
	if(Flag_PC==1)
	{
		if(PORT_Index==1)
		{
			DataTemp[16]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET>>8);
			DataTemp[17]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET&0x00FF);
			
			DataTemp[36]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_Q_SET>>8);
			DataTemp[37]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_Q_SET&0x00FF);
			
			r_eeprom_section_write(1, 16, 2, DataTemp+16);
			
		//	r_eeprom_byte_write(1, 16, DataTemp[16]);
		//	r_eeprom_byte_write(1, 17, DataTemp[17]);
		//	r_eeprom_byte_write(1, 36, DataTemp[36]);
		//	r_eeprom_byte_write(1, 37, DataTemp[37]);
			
			r_eeprom_section_write(1, 36, 2, DataTemp+36);
		}
		
		else if((PORT_Index==2)||(PORT_Index==3))
		{
			DataTemp[16]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET>>8);
			DataTemp[17]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET&0x00FF);
			
			DataTemp[12]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_P_SET>>8);
			DataTemp[13]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_P_SET&0x00FF);
			
			r_eeprom_section_write(1, 16, 2, DataTemp+16);
			
			//r_eeprom_byte_write(1, 16, DataTemp[16]);
			//r_eeprom_byte_write(1, 17, DataTemp[17]);
			//r_eeprom_byte_write(1, 12, DataTemp[12]);
			//r_eeprom_byte_write(1, 13, DataTemp[13]);		
			
			r_eeprom_section_write(1, 12, 2, DataTemp+12);	
		}
	}		
	else
	{
		DataTemp[0]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.COM_SYS_MODE.COM_SYS_MODE16>>8);
		DataTemp[1]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.COM_SYS_MODE.COM_SYS_MODE16&0x00FF);
		
		DataTemp[2]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.AD_PROT_EN.AD_PORT_EN16>>8);
		DataTemp[3]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.AD_PROT_EN.AD_PORT_EN16&0x00FF);
  	
		DataTemp[4]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_1_EN.FO_EN16>>8);
		DataTemp[5]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_1_EN.FO_EN16&0x00FF);
		
		DataTemp[6]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_2_EN.FO_EN16>>8);
		DataTemp[7]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_2_EN.FO_EN16&0x00FF);
		
		DataTemp[8]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_3_EN.FO_EN16>>8);
		DataTemp[9]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FO_3_EN.FO_EN16&0x00FF);
		
		DataTemp[10]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FB_EN.FB_EN16>>8);
		DataTemp[11]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.FB_EN.FB_EN16&0x00FF);
		
		DataTemp[12]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_P_SET>>8);
		DataTemp[13]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_P_SET&0x00FF);
		
		DataTemp[14]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_Q_SET>>8);
		DataTemp[15]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_Q_SET&0x00FF);
  	
		DataTemp[16]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET>>8);
		DataTemp[17]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET&0x00FF);
			
		DataTemp[18]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F_SET>>8);
		DataTemp[19]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F_SET&0x00FF);
			
		DataTemp[20]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.RMS_CURRENT_5TH_SET>>8);
		DataTemp[21]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.RMS_CURRENT_5TH_SET&0x00FF);
		
		DataTemp[22]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.RMS_CURRENT_7TH_SET>>8);
		DataTemp[23]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.RMS_CURRENT_7TH_SET&0x00FF);
		
		DataTemp[24]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.RMS_CURRENT_11TH_SET>>8);
		DataTemp[25]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.RMS_CURRENT_11TH_SET&0x00FF);
			
		DataTemp[26]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PHA_CURRENT_5TH_SET>>8);
		DataTemp[27]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PHA_CURRENT_5TH_SET&0x00FF);
			
		DataTemp[28]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PHA_CURRENT_7TH_SET>>8);
		DataTemp[29]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PHA_CURRENT_7TH_SET&0x00FF);
			
		DataTemp[30]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PHA_CURRENT_11TH_SET>>8);
		DataTemp[31]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PHA_CURRENT_11TH_SET&0x00FF);
			
		DataTemp[32]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_V_SET>>8);
		DataTemp[33]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_V_SET&0x00FF);
			
		DataTemp[34]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_PF_SET>>8);
		DataTemp[35]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_PF_SET&0x00FF);
			
		DataTemp[36]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_Q_SET>>8);
		DataTemp[37]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_Q_SET&0x00FF);
			
		DataTemp[38]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V0_SET>>8);
		DataTemp[39]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V0_SET&0x00FF);
			
		DataTemp[40]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F0_SET>>8);
		DataTemp[41]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F0_SET&0x00FF);
			
		DataTemp[42]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KF_SET>>8);
		DataTemp[43]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KF_SET&0x00FF);
			
		DataTemp[44]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KV_SET>>8);
		DataTemp[45]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KV_SET&0x00FF);
			
		
		for(i=0;i<4;i++)
		{
			DataTemp[46+i*2]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[i]>>8);
			DataTemp[47+i*2]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[i]&0x00FF);
		}

		r_eeprom_section_write(1, 0, 16, DataTemp);
		r_eeprom_section_write(1, 16, 16, DataTemp+16);
		r_eeprom_section_write(1, 32, 16, DataTemp+32);
		r_eeprom_section_write(1, 48, 6, DataTemp+48);
	}
}

/****************************************************************************/
/*                                                                          */
/*              将DSP的数据写入EEPROM的第2页                                */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 把对应端口旧版参数打包写入 EEPROM 第 2 页
 * 输入参数: 无
 * 输出参数: 参数全局量或指定 EEPROM 页。
 * 返回值: 无
 * 执行说明: 旧版存储兼容函数；页映射保持原项目定义。
 *================================================================*/
void DSP_2_EEPROM_Page2(void)
{
  /************************PI参数************************/
  Uint8 DataTemp[24]; /* DataTemp：旧 EEPROM 页的原始字节打包/解包联合缓存。 */
	
	DataTemp[0]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_1>>8);
	DataTemp[1]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_1&0x00FF);
	
	DataTemp[2]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1>>8);
	DataTemp[3]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1&0x00FF);
	
	DataTemp[4]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1>>8);
	DataTemp[5]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1&0x00FF);
	
	DataTemp[6]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_1>>8);
	DataTemp[7]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_1&0x00FF);
	
	DataTemp[8]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_2>>8);
	DataTemp[9]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_2&0x00FF);
	
	DataTemp[10]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_2>>8);
	DataTemp[11]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_2&0x00FF);
	
	DataTemp[12]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_2>>8);
	DataTemp[13]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_2&0x00FF);
	
	DataTemp[14]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_2>>8);
	DataTemp[15]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_2&0x00FF);
	
	DataTemp[16]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_3>>8);
	DataTemp[17]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_3&0x00FF);
	
	DataTemp[18]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_3>>8);
	DataTemp[19]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_3&0x00FF);
	
	DataTemp[20]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_3>>8);
	DataTemp[21]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_3&0x00FF);
	
	DataTemp[22]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_3>>8);
	DataTemp[23]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_3&0x00FF);
		
	r_eeprom_section_write(2, 0, 16, DataTemp);
	r_eeprom_section_write(2, 16, 8, DataTemp+16);
}

/****************************************************************************/
/*                                                                          */
/*              将DSP的数据写入EEPROM的第3页                            */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 把对应端口旧版参数打包写入 EEPROM 第 3 页
 * 输入参数: 无
 * 输出参数: 参数全局量或指定 EEPROM 页。
 * 返回值: 无
 * 执行说明: 旧版存储兼容函数；页映射保持原项目定义。
 *================================================================*/
void DSP_2_EEPROM_Page3(void)
{
	//读EEPROM的第3页的遥调量	
	/************************EER2的数据************************/
	
	Uint8 DataTemp[8]; /* DataTemp：旧 EEPROM 页的原始字节打包/解包联合缓存。 */
	Uint8 i; /* i：本次数组、参数或模块遍历的索引。 */
	
	for (i=0;i<4;i++) 
	{
		DataTemp[2*i]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.EER2_DATA[i]>>8);
		DataTemp[2*i+1]=(Uint8)(PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.EER2_DATA[i]&0x00FF);
	}
	r_eeprom_section_write(3, 0, 8, DataTemp);
}

//===========================================================================
// End of file.
//===========================================================================
