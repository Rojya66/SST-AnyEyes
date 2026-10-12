#include "SstControlConfig.h"
//###########################################################################
// FILE: DacOutput.c
// FUNCTION: 四通道DAC硬件输出。
//           原函数名、函数正文和条件编译保持；无算法改动。
//###########################################################################
#include "ControlData.h"
#include "DpramHostData.h"
#include "DpramRegister.h"
#include "FpgaRegister.h"
#include "Ft3RecorderData.h"
#include "MeasurementData.h"
#include "PortStatusData.h"
#include "ProtectionData.h"
#include "SstBoardConfig.h"
#include "SstTypes.h"

#include "DacOutput.h"
#include "ChbBuildConfig.h"

/*==================================================================
 * 函数功能: 将 DAC_BUF 四个通道值写入主控插件的 DA 寄存器
 * 输入参数: 无
 * 输出参数: 写函数修改对应 FPGA 寄存器或控制字缓存；读函数通过返回值提供原始数据。
 * 返回值: 无
 * 执行说明: 底层 FPGA 寄存器访问；插件编号沿用原工程，不能改变地址或结构体布局。
 *================================================================*/
void DAC_Output(void)
{
	unsigned char i; /* i：本次数组、参数或模块遍历的索引。 */
	float tempDAC; /* tempDAC：单通道 DA 输出缩放、限幅后的计数缓存。 */
	
	for(i=0;i<4;i++)
	{
		if(DAC_BUF[i]>5.0)
		{
			DAC_BUF[i]=5.0;
		}

		if(DAC_BUF[i]<0)
		{
			DAC_BUF[i]=0;
		}
		
		tempDAC=DAC_BUF[i]*4095.0/5.0;
		
		DAC_BUF[i]=tempDAC;
	}
	
//tempInt=-SIN_Vin;
//tempInt=tempInt*10.0+10.0;
//DAC_BUF[3]=tempInt/2.0;
	
	FPGA_Interface[MainControl_PluginUnit_4-2].DA_DATA0=(unsigned short)DAC_BUF[0];
	FPGA_Interface[MainControl_PluginUnit_4-2].DA_DATA1=(unsigned short)DAC_BUF[1];
	FPGA_Interface[MainControl_PluginUnit_4-2].DA_DATA2=(unsigned short)DAC_BUF[2];
	FPGA_Interface[MainControl_PluginUnit_4-2].DA_DATA3=(unsigned short)DAC_BUF[3];
}

