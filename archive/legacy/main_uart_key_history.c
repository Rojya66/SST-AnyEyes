/*
void Uart2_send(unsigned char PCcomm)
{
        unsigned int txEmptytt;
    unsigned char i;
    unsigned char length;
    unsigned short CRC_Temp;

    GPIOPinWrite(SOC_GPIO_0_REGS, 104, 1);

        length=0;
        PCuartBUFF[length++]=ControllerAddr;
        PCuartBUFF[length++]=CommFunc;

        switch(CommFunc)
        {
            case WriteController:
            {
                PCuartBUFF[length++]=WriteOK;
            }break;

            case ReadController:
            {
                PCuartBUFF[length++]=StartAddr;
                PCuartBUFF[length++]=StartAddr>>8;
                PCuartBUFF[length++]=StartAddr>>16;
                PCuartBUFF[length++]=StartAddr>>24;
                PCuartBUFF[length++]=0;
                PCuartBUFF[length++]=ReadLength;
                length=length+ReadLength+ReadLength;

            }break;

            default: break;
        }

        CRC_Temp=CRC16(&PCuartBUFF[0], length);

        PCuartBUFF[length++]=CRC_Temp>>8;
        PCuartBUFF[length++]=CRC_Temp;


        for(i=0;i<length;i++)
    {
        UARTCharPut(SOC_UART_2_REGS, PCuartBUFF[i]);
    }


   txEmptytt = (UART_THR_TSR_EMPTY | UART_THR_EMPTY);


   while (txEmptytt != (HWREG(SOC_UART_2_REGS + UART_LSR) & txEmptytt));

   GPIOPinWrite(SOC_GPIO_0_REGS, 104, 0);
}
*/

/****************************************************************************/
/*                                                                          */
/*              旧版UART接收处理 - 已废弃，使用VOFA替代                        */
/*                                                                          */
/****************************************************************************/
#ifdef HVAC_online
#if 0
void UARTReceiveDispose(void)
{
{
    unsigned short CRC_Temp_1,CRC_Temp_2,WriteD,i; /* CRC_Temp_1：旧维护帧 CRC 校验的第一缓存字。 */
    /* CRC_Temp_2：旧维护帧 CRC 校验的第二缓存字。 */
    /* WriteD：旧维护写入数据/状态的兼容变量。 */
    /* i：本次数组、参数或模块遍历的索引。 */
    unsigned int Addr_write_temp; /* Addr_write_temp：旧维护帧写操作的临时地址。 */
    unsigned char i_temp; /* i_temp：旧初始化/维护流程临时整字计算值。 */
    ushort InitialTemp,InitialTemp_2; /* InitialTemp：原模块配置/控制字初始化的整字计算缓存。 */
    /* InitialTemp_2：原模块配置初始化的附加整字计算缓存。 */
    float InitialTempFloat; /* InitialTempFloat：原控制系数和给定初始化的浮点计算缓存。 */

#if CHB_OPEN_LOOP_PULSE_TEST
    /* 开环发脉冲测试修改：透明桥模式。
     * 原程序这里包含大量 0x60000620 特殊命令，会改 PM/NUM/闭环参数；
     * 测试版全部绕过，只允许调试界面按实际外部地址读写 CS2。
     */
    CRC_Temp_1=CRC16(&ReceiveuartBUFF[0],CommTotalNum);

    CRC_Temp_2=(unsigned short)ReceiveuartBUFF[CommTotalNum];
    CRC_Temp_2=CRC_Temp_2<<8;
    CRC_Temp_2=CRC_Temp_2|(unsigned short)ReceiveuartBUFF[CommTotalNum+1];

    if(CRC_Temp_1!=CRC_Temp_2)
    {
        return;
    }

    if((StartAddr<ValidAddr_CS2_S)||(StartAddr>ValidAddr_CS2_E))
    {
        return;
    }

    Addr_write_temp=StartAddr;
    if(CommFunc==WriteController)
    {
        WriteD=ReceiveuartBUFF[7];
        WriteD=WriteD<<8;
        WriteD=WriteD+ReceiveuartBUFF[6];

        HWREGH(Addr_write_temp)=WriteD;
        if(WriteD==HWREGH(Addr_write_temp))
        {
            WriteOK=0x28;
        }
        else
        {
            WriteOK=0x19;
        }
        /* 旧版上位机通讯调用已废弃 */
        // Uart2_send(WriteController);
    }
    else
    {
        for(i=0;i<ReadLength;i++)
        {
            WriteD=HWREGH(Addr_write_temp);
            Addr_write_temp=Addr_write_temp+2;

            i_temp=(unsigned char)(i<<1);
            PCuartBUFF[i_temp+8]=WriteD;
            PCuartBUFF[i_temp+9]=WriteD>>8;
        }
        /* 旧版上位机通讯调用已废弃 */
        // Uart2_send(ReadController);
    }
    return;
#endif

    CRC_Temp_1=CRC16(&ReceiveuartBUFF[0],CommTotalNum);

    CRC_Temp_2=(unsigned short)ReceiveuartBUFF[CommTotalNum];
    CRC_Temp_2=CRC_Temp_2<<8;
    CRC_Temp_2=CRC_Temp_2|(unsigned short)ReceiveuartBUFF[CommTotalNum+1];

    if(CRC_Temp_1!=CRC_Temp_2)
    {
        return;
    }

    if((StartAddr<ValidAddr_CS2_S)||(StartAddr>ValidAddr_CS5_E))
    {
        return;
    }
    else if((StartAddr>ValidAddr_CS2_E)&&(StartAddr<ValidAddr_CS5_S))
    {
        return;
    }
    else
    {
        Addr_write_temp=StartAddr;

        if(CommFunc==WriteController)
        {
            WriteD=ReceiveuartBUFF[7];
            WriteD=WriteD<<8;
            WriteD=WriteD+ReceiveuartBUFF[6];

            if((Addr_write_temp==0x60000620)&&(WriteD==0x1100))
            {
                Flag_ComTest=0;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0x1101))
            {
                Flag_ComTest=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0x1200))
            {
                FourPortComZhi=WriteD&0x00ff;
            }


            //模块配置
            /*
             * Online module-configuration command.
             * 0xe1xx..0xe6xx update PM for A1/A2/B1/B2/C1/C2.
             * After any PM change, recompute the two-card phase NUM and
             * ReferenceDEF, then write PM/NUM back through FPGA_Interface.
             */
            if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xe100))
            {
                HA_PM_A1=WriteD&0x00ff;
                FPGA_Interface[Com_A1_PluginUnit_5-2].PM=HA_PM_A1;

                NHA_module_A=0;
                for(i=0;i<8;i++)
                {
                    InitialTemp=HA_PM_A1;
                    if(InitialTemp&(0x0001<<i))
                    {
                        NHA_module_A++;
                    }
                    InitialTemp=HA_PM_A2;
                    if(InitialTemp&(0x0001<<i))
                    {
                        NHA_module_A++;
                    }
                }
                FPGA_Interface[Com_A1_PluginUnit_5-2].NUM=NHA_module_A;
                FPGA_Interface[Com_A2_PluginUnit_6-2].NUM=NHA_module_A;

                InitialTemp=NHA_module_A;
                HA_ReferenceDEF_A=(float)InitialTemp;
                InitialTempFloat=ReferenceDEF;
                HA_ReferenceDEF_A=HA_ReferenceDEF_A*InitialTempFloat;

                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[0]=HA_PM_A1;

#ifdef EEPROM_ON
                DSP_2_EEPROM_Page1();
#endif

                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xe200))
            {
                HA_PM_A2=WriteD&0x00ff;
                FPGA_Interface[Com_A2_PluginUnit_6-2].PM=HA_PM_A2;

                NHA_module_A=0;
                for(i=0;i<8;i++)
                {
                    InitialTemp=HA_PM_A1;
                    if(InitialTemp&(0x0001<<i))
                    {
                        NHA_module_A++;
                    }
                    InitialTemp=HA_PM_A2;
                    if(InitialTemp&(0x0001<<i))
                    {
                        NHA_module_A++;
                    }
                }
                FPGA_Interface[Com_A1_PluginUnit_5-2].NUM=NHA_module_A;
                FPGA_Interface[Com_A2_PluginUnit_6-2].NUM=NHA_module_A;

                InitialTemp=NHA_module_A;
                HA_ReferenceDEF_A=(float)InitialTemp;
                InitialTempFloat=ReferenceDEF;
                HA_ReferenceDEF_A=HA_ReferenceDEF_A*InitialTempFloat;

                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[1]=HA_PM_A2;

#ifdef EEPROM_ON
                DSP_2_EEPROM_Page1();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xe300))
            {
                HA_PM_B1=WriteD&0x00ff;
                FPGA_Interface[Com_B1_PluginUnit_9-2].PM=HA_PM_B1;

                NHA_module_B=0;
                for(i=0;i<8;i++)
                {
                    InitialTemp=HA_PM_B1;
                    if(InitialTemp&(0x0001<<i))
                    {
                        NHA_module_B++;
                    }
                    InitialTemp=HA_PM_B2;
                    if(InitialTemp&(0x0001<<i))
                    {
                        NHA_module_B++;
                    }
                }
                FPGA_Interface[Com_B1_PluginUnit_9-2].NUM=NHA_module_B;
                FPGA_Interface[Com_B2_PluginUnit_10-2].NUM=NHA_module_B;

                InitialTemp=NHA_module_B;
                HA_ReferenceDEF_B=(float)InitialTemp;
                InitialTempFloat=ReferenceDEF;
                HA_ReferenceDEF_B=HA_ReferenceDEF_B*InitialTempFloat;

                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V0_SET=HA_PM_B1;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page1();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xe400))
            {
                HA_PM_B2=WriteD&0x00ff;
                FPGA_Interface[Com_B2_PluginUnit_10-2].PM=HA_PM_B2;

                NHA_module_B=0;
                for(i=0;i<8;i++)
                {
                    InitialTemp=HA_PM_B1;
                    if(InitialTemp&(0x0001<<i))
                    {
                        NHA_module_B++;
                    }
                    InitialTemp=HA_PM_B2;
                    if(InitialTemp&(0x0001<<i))
                    {
                        NHA_module_B++;
                    }
                }
                FPGA_Interface[Com_B1_PluginUnit_9-2].NUM=NHA_module_B;
                FPGA_Interface[Com_B2_PluginUnit_10-2].NUM=NHA_module_B;

                InitialTemp=NHA_module_B;
                HA_ReferenceDEF_B=(float)InitialTemp;
                InitialTempFloat=ReferenceDEF;
                HA_ReferenceDEF_B=HA_ReferenceDEF_B*InitialTempFloat;

                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F0_SET=HA_PM_B2;
                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F0_SET=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_F0_SET*10;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page1();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xe500))
            {
                HA_PM_C1=WriteD&0x00ff;
                FPGA_Interface[Com_C1_PluginUnit_13-2].PM=HA_PM_C1;

                NHA_module_C=0;
                for(i=0;i<8;i++)
                {
                    InitialTemp=HA_PM_C1;
                    if(InitialTemp&(0x0001<<i))
                    {
                        NHA_module_C++;
                    }
                    InitialTemp=HA_PM_C2;
                    if(InitialTemp&(0x0001<<i))
                    {
                        NHA_module_C++;
                    }
                }
                FPGA_Interface[Com_C1_PluginUnit_13-2].NUM=NHA_module_C;
                FPGA_Interface[Com_C2_PluginUnit_14-2].NUM=NHA_module_C;

                InitialTemp=NHA_module_C;
                HA_ReferenceDEF_C=(float)InitialTemp;
                InitialTempFloat=ReferenceDEF;
                HA_ReferenceDEF_C=HA_ReferenceDEF_C*InitialTempFloat;

                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KF_SET=HA_PM_C1;
                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KF_SET=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KF_SET*10;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page1();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xe600))
            {
                HA_PM_C2=WriteD&0x00ff;
                FPGA_Interface[Com_C2_PluginUnit_14-2].PM=HA_PM_C2;

                NHA_module_C=0;
                for(i=0;i<8;i++)
                {
                    InitialTemp=HA_PM_C1;
                    if(InitialTemp&(0x0001<<i))
                    {
                        NHA_module_C++;
                    }
                    InitialTemp=HA_PM_C2;
                    if(InitialTemp&(0x0001<<i))
                    {
                        NHA_module_C++;
                    }
                }
                FPGA_Interface[Com_C1_PluginUnit_13-2].NUM=NHA_module_C;
                FPGA_Interface[Com_C2_PluginUnit_14-2].NUM=NHA_module_C;

                InitialTemp=NHA_module_C;
                HA_ReferenceDEF_C=(float)InitialTemp;
                InitialTempFloat=ReferenceDEF;
                HA_ReferenceDEF_C=HA_ReferenceDEF_C*InitialTempFloat;

                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KV_SET=HA_PM_C2;
                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KV_SET=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_KV_SET*10;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page1();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else
            {}

            //Flag_Switch//1: 动接触器；2：动内部断路器;3：动外部断路器1；4：动外部断路器2
            if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xfa00))
            {
                SW.Bbits.Flag_Switch=1;
                SW.Bbits.Flag_SwitchZhi=WriteD&0x0001;
                SW.Bbits.Flag_SwitchRun=0;
                SW.Bbits.Flag_SwitchSuccess=0;
                SWTable_TotalTime=SwitchTotalNum;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xfb00))
            {
                SW.Bbits.Flag_SwitchZhi=WriteD&0x0001;

                if(SW.Bbits.Flag_SwitchZhi==1)
                {
                    Port_DINRead(0);
                    if(SW.Bbits.S_CONTACTOR==0)
                    {
                        SW.Bbits.Flag_Switch=2;
                        SW.Bbits.Flag_SwitchRun=0;
                        SW.Bbits.Flag_SwitchSuccess=0;
                        SWTable_TotalTime=SwitchTotalNum;
                    }
                }
                else
                {
                    SW.Bbits.Flag_Switch=2;
                    SW.Bbits.Flag_SwitchRun=0;
                    SW.Bbits.Flag_SwitchSuccess=0;
                    SWTable_TotalTime=SwitchTotalNum;
                }
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xfc00))
            {
                SW.Bbits.Flag_Switch=3;
                SW.Bbits.Flag_SwitchZhi=WriteD&0x0001;
                SW.Bbits.Flag_SwitchRun=0;
                SW.Bbits.Flag_SwitchSuccess=0;
                SWTable_TotalTime=SwitchTotalNum;
            }
            else
            {}

            //运行方式
            if((Addr_write_temp==0x60000620)&&(WriteD==0xFF00))
            {
                Port_RealStatus.Bbits.S_Mode=2;   //手动

                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.COM_SYS_MODE.Bbits.HA_RunMode=1;
                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.COM_SYS_MODE.Bbits.HA_CtlMode=0;

#ifdef EEPROM_ON
                DSP_2_EEPROM_Page1();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xFF01))
            {
                Port_RealStatus.Bbits.S_Mode=0;   //自动

                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.COM_SYS_MODE.Bbits.HA_RunMode=1;
                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.COM_SYS_MODE.Bbits.HA_CtlMode=1;

#ifdef EEPROM_ON
                DSP_2_EEPROM_Page1();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else
            {}

            //工作模式
            if((Addr_write_temp==0x60000620)&&(WriteD==0xff10))
            {
                Port_STATCOM_WorkMode=0;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page0();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xff11))
            {
                Port_STATCOM_WorkMode=1;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page0();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xff12))
            {
                Port_STATCOM_WorkMode=2;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page0();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xff20))
            {
                Port_APF_WorkMode=0;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page0();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xff21))
            {
                Port_APF_WorkMode=1;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page0();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xff22))
            {
                Port_APF_WorkMode=2;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page0();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else
            {}


            if((Addr_write_temp==0x60000620)&&(WriteD==0xf2a0))
            {
                Add_3th=0;
                HWREGH(0x60000624)=0;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xf2a1))
            {
                Add_3th=1;
                HWREGH(0x60000624)=1;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xf2aa))
            {
                Method_3th=1;
                HWREGH(0x60000624)=11;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xf2ab))
            {
                Method_3th=2;
                HWREGH(0x60000624)=21;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xf2ac))
            {
                Method_3th=3;
                HWREGH(0x60000624)=31;
            }
            else
            {}

            //风扇启停
            if((Addr_write_temp==0x60000620)&&(WriteD==0xf3a0))
            {
                Module_FAN_OFF(1);
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xf3a1))
            {
                Module_FAN_OFF(0);
            }
            else
            {}

            //标幺化
            if((Addr_write_temp==0x60000620)&&(WriteD==0xf4a0))
            {
                flag_Standard=0;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xf4a1))
            {
                flag_Standard=1;
            }
            else
            {}

            //故障闭锁设置
            if((Addr_write_temp==0x60000620)&&(WriteD==0xf5a0))
            {
                flag_FaultClose=0;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xf5a1))
            {
                flag_FaultClose=1;
            }
            else
            {}


            //无功设置
            if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xc000))
            {
                InitialTemp=WriteD&0x000f;
                if(InitialTemp>9)
                {
                    InitialTemp=0;
                }
                InitialTemp_2=(WriteD&0x00f0)>>4;
                if(InitialTemp_2>9)
                {
                    InitialTemp_2=0;
                }
                InitialTemp_2=InitialTemp_2*10+InitialTemp;


                #ifdef SVG_10KV
//////////////////////////////////////////////////////////
                if(InitialTemp_2>60)
                {
                    InitialTemp_2=60;
                }
//////////////////////////////////////////////////////////
                HWREGH(0x6000062c)=InitialTemp_2;
                InitialTempFloat=(float)InitialTemp_2;
                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_Q_SET=InitialTemp_2*200;
                PORT_Regulate_DATA.STATCOM_Q_SET=InitialTempFloat*20000.0;
                #endif

                #ifdef SVG_380V
//////////////////////////////////////////////////////////
                if(InitialTemp_2>4)
                {
                    InitialTemp_2=4;
                }
//////////////////////////////////////////////////////////
                InitialTempFloat=(float)InitialTemp_2;
                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_Q_SET=InitialTemp_2*50;
                PORT_Regulate_DATA.STATCOM_Q_SET=InitialTempFloat*5000.0;
                #endif

#ifdef EEPROM_ON
                DSP_2_EEPROM_Page1();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xc100))
            {
                InitialTemp=WriteD&0x000f;
                if(InitialTemp>9)
                {
                    InitialTemp=0;
                }
                InitialTemp_2=(WriteD&0x00f0)>>4;
                if(InitialTemp_2>9)
                {
                    InitialTemp_2=0;
                }
                InitialTemp_2=InitialTemp_2*10+InitialTemp;


                #ifdef SVG_10KV
//////////////////////////////////////////////////////////
                if(InitialTemp_2>60)
                {
                    InitialTemp_2=60;
                }
//////////////////////////////////////////////////////////
                HWREGH(0x6000062c)=InitialTemp_2;
                InitialTempFloat=(float)InitialTemp_2;
                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_Q_SET=InitialTemp_2*(-200);
                PORT_Regulate_DATA.STATCOM_Q_SET=InitialTempFloat*(-20000.0);
                #endif

                #ifdef SVG_380V
//////////////////////////////////////////////////////////
                if(InitialTemp_2>4)
                {
                    InitialTemp_2=4;
                }
//////////////////////////////////////////////////////////
                InitialTempFloat=(float)InitialTemp_2;
                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_Q_SET=InitialTemp_2*(-50);
                PORT_Regulate_DATA.STATCOM_Q_SET=InitialTempFloat*(-5000.0);
                #endif

#ifdef EEPROM_ON
                DSP_2_EEPROM_Page1();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xc200))
            {
                InitialTemp=WriteD&0x00ff;
                if(InitialTemp>9)
                {
                    InitialTemp=0;
                }
                InitialTemp_2=(WriteD&0x00f0)>>4;
                if(InitialTemp_2>9)
                {
                    InitialTemp_2=0;
                }

                InitialTemp_2=InitialTemp_2*10+InitialTemp;
                if(InitialTemp_2==0)
                {
                    InitialTemp_2=100;
                }
                else if(InitialTemp_2<70)
                {
                    InitialTemp_2=100;
                }
                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.STATCOM_PF_SET=InitialTemp_2;
                PORT_Regulate_DATA.STATCOM_PF_SET=(float)InitialTemp_2/100.0;

#ifdef EEPROM_ON
                DSP_2_EEPROM_Page1();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else
            {}


            //参考Udc 和Idmax
            if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xa000))
            {
                InitialTemp=WriteD&0x000f;
                if(InitialTemp>9)
                {
                    InitialTemp=0;
                }
                InitialTemp_2=(WriteD&0x00f0)>>4;
                if(InitialTemp_2>9)
                {
                    InitialTemp_2=0;
                }
                InitialTemp_2=InitialTemp_2*10+InitialTemp;

                if(InitialTemp_2>75)
                {
                    InitialTemp_2=70;
                }
                else if(InitialTemp_2<15)
                {
                    InitialTemp_2=70;
                }
                else
                {}

                HA_Module_RefUdc=(float)InitialTemp_2*10.0;
                UstOK_HVAC_Port=HA_Module_RefUdc;
                Udc_Ref=HA_Module_RefUdc;

                if(NHA_module_A==0)
                {
                    Ud_st_Max=10.0;
                    Ud_st_Min=-10.0;
                    Uq_st_Max=10.0;
                    Uq_st_Min=-10.0;
                }
                else
                {
                    InitialTempFloat=(float)NHA_module_A;
                    InitialTempFloat=InitialTempFloat*1.1;
                    InitialTempFloat=InitialTempFloat*HA_Module_RefUdc;

                    Ud_st_Max=InitialTempFloat;
                    Ud_st_Min=InitialTempFloat*(-1.0);
                    Uq_st_Max=Ud_st_Max;
                    Uq_st_Min=Ud_st_Min;
                }

                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET=(ushort)HA_Module_RefUdc;

                HWREGH(0x6000062A)=PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_SET;

                Uout_d_Positive_Max=Ud_st_Max;
                Uout_d_Positive_Min=Ud_st_Min;
                Uout_q_Positive_Max=Uq_st_Max;
                Uout_q_Positive_Min=Uq_st_Min;

#ifdef EEPROM_ON
                DSP_2_EEPROM_Page1();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xa100))
            {
                InitialTemp=WriteD&0x000f;
                if(InitialTemp>9)
                {
                    InitialTemp=0;
                }
                InitialTemp_2=(WriteD&0x00f0)>>4;
                if(InitialTemp_2>9)
                {
                    InitialTemp_2=0;
                }
                InitialTemp_2=InitialTemp_2*10+InitialTemp;

                if(InitialTemp_2>60)
                {
                    InitialTemp_2=60;
                }
                else if(InitialTemp_2<10)
                {
                    InitialTemp_2=10;
                }
                else
                {}
                HWREGH(0x60000a2C)=InitialTemp_2;
                Idst_Max=(float)InitialTemp_2*2.0;
                Idst_Min=(-1.0)*Idst_Max;
                Iqst_Max=Idst_Max;
                Iqst_Min=Idst_Min;

                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[3]=(ushort)Idst_Max;


                Id_Positive_Max=Idst_Max;
                Id_Positive_Min=Idst_Min;
                Iq_Positive_Max=Iqst_Max;
                Iq_Positive_Min=Iqst_Min;

#ifdef EEPROM_ON
                DSP_2_EEPROM_Page1();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else{}

            //PI
            if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xB000))  //PLL
            {
                InitialTemp=WriteD&0x000f;
                if(InitialTemp>9)
                {
                    InitialTemp=0;
                }
                InitialTemp_2=(WriteD&0x00f0)>>4;
                if(InitialTemp_2>9)
                {
                    InitialTemp_2=0;
                }
                InitialTemp_2=InitialTemp_2*10+InitialTemp;

                if(InitialTemp_2==0)
                {
                    PLL_Factor=1.0;
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_1=100;
                }
                else
                {
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_1=InitialTemp_2*10;
                    PLL_Factor=(float)InitialTemp_2/10.0;
                }
                PLL_PID_Num_1=PLL_PID_Num_1_def/PLL_Factor;
                PLL_PID_Num_0=PLL_PID_Num_0_def/PLL_Factor;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page2();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xB100))  //Voltage
            {
                InitialTemp=WriteD&0x000f;
                if(InitialTemp>9)
                {
                    InitialTemp=0;
                }
                InitialTemp_2=(WriteD&0x00f0)>>4;
                if(InitialTemp_2>9)
                {
                    InitialTemp_2=0;
                }
                InitialTemp_2=InitialTemp_2*10+InitialTemp;

                if(InitialTemp_2==0)
                {
                    Volt_Factor=1.0;
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1=100;
                }
                else
                {
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_1=InitialTemp_2*10;
                    Volt_Factor=(float)InitialTemp_2/10.0;
                }
                Volt_PID_Num_1=Volt_PID_Num_1_def/Volt_Factor;
                Volt_PID_Num_0=Volt_PID_Num_0_def/Volt_Factor;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page2();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xB200))  //Current
            {
                InitialTemp=WriteD&0x000f;
                if(InitialTemp>9)
                {
                    InitialTemp=0;
                }
                InitialTemp_2=(WriteD&0x00f0)>>4;
                if(InitialTemp_2>9)
                {
                    InitialTemp_2=0;
                }
                InitialTemp_2=InitialTemp_2*10+InitialTemp;

                if(InitialTemp_2==0)
                {
                    Current_Factor=1.0;
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1=100;
                }
                else
                {
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_1=InitialTemp_2*10;
                    Current_Factor=(float)InitialTemp_2/10.0;
                }
                Current_PID_Num_1=Current_PID_Num_1_def/Current_Factor;
                Current_PID_Num_0=Current_PID_Num_0_def/Current_Factor;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page2();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xB300))  //Neg_Current
            {
                InitialTemp=WriteD&0x000f;
                if(InitialTemp>9)
                {
                    InitialTemp=0;
                }
                InitialTemp_2=(WriteD&0x00f0)>>4;
                if(InitialTemp_2>9)
                {
                    InitialTemp_2=0;
                }
                InitialTemp_2=InitialTemp_2*10+InitialTemp;

                if(InitialTemp_2==0)
                {
                    Neg_Current_Factor=1.0;
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_1=100;
                }
                else
                {
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_1=InitialTemp_2*10;
                    Neg_Current_Factor=(float)InitialTemp_2/10.0;
                }
                Neg_Current_PID_Num_1=Neg_Current_PID_Num_1_def/Neg_Current_Factor;
                Neg_Current_PID_Num_0=Neg_Current_PID_Num_0_def/Neg_Current_Factor;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page2();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xB400))  //PH_Volt
            {
                InitialTemp=WriteD&0x000f;
                if(InitialTemp>9)
                {
                    InitialTemp=0;
                }
                InitialTemp_2=(WriteD&0x00f0)>>4;
                if(InitialTemp_2>9)
                {
                    InitialTemp_2=0;
                }
                InitialTemp_2=InitialTemp_2*10+InitialTemp;

                if(InitialTemp_2==0)
                {
                    PH_Volt_Factor=1.0;
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_2=100;
                }
                else
                {
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_P_2=InitialTemp_2*10;
                    PH_Volt_Factor=(float)InitialTemp_2/10.0;
                }
                PH_Volt_PID_Num_1=PH_Volt_PID_Num_1_def/PH_Volt_Factor;
                PH_Volt_PID_Num_0=PH_Volt_PID_Num_0_def/PH_Volt_Factor;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page2();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xB500))  //Har_2
            {
                InitialTemp=WriteD&0x000f;
                if(InitialTemp>9)
                {
                    InitialTemp=0;
                }
                InitialTemp_2=(WriteD&0x00f0)>>4;
                if(InitialTemp_2>9)
                {
                    InitialTemp_2=0;
                }
                InitialTemp_2=InitialTemp_2*10+InitialTemp;

                if(InitialTemp_2==0)
                {
                    Har_2_Factor=1.0;
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_2=100;
                }
                else
                {
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_V_I_2=InitialTemp_2*10;
                    Har_2_Factor=(float)InitialTemp_2/10.0;
                }
                Har_2_Controller_Num_2=Har_2_Controller_Num_2_def/Har_2_Factor;
                Har_2_Controller_Num_1=Har_2_Controller_Num_1_def/Har_2_Factor;
                Har_2_Controller_Num_0=Har_2_Controller_Num_0_def/Har_2_Factor;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page2();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xB600))  //Har_6
            {
                InitialTemp=WriteD&0x000f;
                if(InitialTemp>9)
                {
                    InitialTemp=0;
                }
                InitialTemp_2=(WriteD&0x00f0)>>4;
                if(InitialTemp_2>9)
                {
                    InitialTemp_2=0;
                }
                InitialTemp_2=InitialTemp_2*10+InitialTemp;

                if(InitialTemp_2==0)
                {
                    Har_6_Factor=1.0;
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_2=100;
                }
                else
                {
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_P_2=InitialTemp_2*10;
                    Har_6_Factor=(float)InitialTemp_2/10.0;
                }
                Har_6_Controller_Num_2=Har_6_Controller_Num_2_def/Har_6_Factor;
                Har_6_Controller_Num_1=Har_6_Controller_Num_1_def/Har_6_Factor;
                Har_6_Controller_Num_0=Har_6_Controller_Num_0_def/Har_6_Factor;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page2();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&((WriteD&0xff00)==0xB700))  //Har_12
            {
                InitialTemp=WriteD&0x000f;
                if(InitialTemp>9)
                {
                    InitialTemp=0;
                }
                InitialTemp_2=(WriteD&0x00f0)>>4;
                if(InitialTemp_2>9)
                {
                    InitialTemp_2=0;
                }
                InitialTemp_2=InitialTemp_2*10+InitialTemp;

                if(InitialTemp_2==0)
                {
                    Har_12_Factor=1.0;
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_2=100;
                }
                else
                {
                    PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.PORT_I_I_2=InitialTemp_2*10;
                    Har_12_Factor=(float)InitialTemp_2/10.0;
                }
                Har_12_Controller_Num_2=Har_12_Controller_Num_2_def/Har_12_Factor;
                Har_12_Controller_Num_1=Har_12_Controller_Num_1_def/Har_12_Factor;
                Har_12_Controller_Num_0=Har_12_Controller_Num_0_def/Har_12_Factor;
#ifdef EEPROM_ON
                DSP_2_EEPROM_Page2();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xB85A))
            {
                //Current loop
                flag_INV=1;
                HWREGH(0x60000628)=1;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xB8EA))
            {
                //Current loop
                flag_INV=0;
                HWREGH(0x60000628)=0;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xA85A))
            {
                DAB_ONLY=1;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xA8EA))
            {
                DAB_ONLY=0;
            }
            else{}

            //端口在线
            if((Addr_write_temp==0x60000620)&&((WriteD&0xAAF0)==0xAA00))
            {
                Port_OnState=WriteD&0x000f;
                InitialTemp=4-PORT_Index;
                InitialTemp=0x0001<<InitialTemp;
                Port_OnState=Port_OnState|InitialTemp;
                HWREGH(0x6000062e)=Port_OnState;

                PORT_Regulate_DATA_Buf.DPRAM_RemRegulate.Reserved_1[2]=Port_OnState;

#ifdef EEPROM_ON
                DSP_2_EEPROM_Page1();
#endif
                FLAG_RemRegulate_SetOver=1;
            }
            else{}

            //脉冲操作
            if((Addr_write_temp==0x60000620)&&(WriteD==0xff50))
            {
                ControlFLAG.Bbits.Port_Stop=0x00;
                ControlFLAG.Bbits.Port_Start=0x01;
                Port_RunStatus=PortStatusNum_Start;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xff51))
            {
                ControlFLAG.Bbits.Port_Stop=0x01;
                ControlFLAG.Bbits.Port_Start=0x00;

                Port_RunStatus=PortStatusNum_Stop;

                ControlFLAG.Bbits.SVG_Start=0;
                FaultDisposeFLAG.Bbits.FaultDisposeType=2;
                FaultDisposeFLAG.Bbits.PWMPulseState=3;
                FaultDisposeFLAG.Bbits.Port_StopType=3;    //000: non-stop   001:normal shutdown  002: system shutdown   003: Remote-control shutdown

            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xff5a))
            {
                FaultLatch_Clear(0,1);
/*
//              EEPROM_Page1_2_DSP();
//              RemoteRegulate_Dispose(1);
//              flag_SendDABRef_First=1;
//              flag_SendSVGRef_First=1;
//              ControlFLAG.Bbits.DAB_DutyStart=1;
//              ControlFLAG.Bbits.DAB_DutyOver=0;
//              HA_DABDuty_A=250;    //5%
//              Write_ControlParameter_DAB_Startup();
//              PWMPluse_Enable(WriteD&0x00ff);
*/

                flag_DABOPEN=1;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xff5b))
            {
                FaultDisposeFLAG.Bbits.PWMPulseState=0x02;
                PWMPluse_Disable(0);

            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffaa))
            {
                Port_DINRead(0);
                if(SW.Bbits.S_CONTACTOR==1)
                {
                    FaultLatch_Clear(0,1);
                    flag_SVGOPEN=1;
//              Variable_RST();
                }
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffab))
            {
                FaultDisposeFLAG.Bbits.PWMPulseState=0x01;
                PWMPluse_Disable(0);
            }
            else
            {}

            if((Addr_write_temp==0x60000620)&&(WriteD==0xffc0))
            {
                Port_BKU_Send(1);    //send BKU
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffc1))
            {
                Port_BKU_Send(0);    //send BKU
            }

/*
            if((Addr_write_temp==0x60000620)&&(WriteD==0xffc0))
            {
                PORT_Regulate_DATA.RMS_CURRENT_5TH_SET=0.0;
                PORT_Regulate_DATA.RMS_CURRENT_7TH_SET=0.0;
                PORT_Regulate_DATA.RMS_CURRENT_11TH_SET=0.0;
                PORT_Regulate_DATA.PHA_CURRENT_5TH_SET=0.0;
                PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=0.0;
                PORT_Regulate_DATA.PHA_CURRENT_11TH_SET=0.0;

                Har_5_RMS=PORT_Regulate_DATA.RMS_CURRENT_5TH_SET;
                Har_5_PH=PORT_Regulate_DATA.PHA_CURRENT_5TH_SET;
                Har_7_RMS=PORT_Regulate_DATA.RMS_CURRENT_7TH_SET;
                Har_7_PH=PORT_Regulate_DATA.PHA_CURRENT_7TH_SET;
                Har_11_RMS=PORT_Regulate_DATA.RMS_CURRENT_11TH_SET;
                Har_11_PH=PORT_Regulate_DATA.PHA_CURRENT_11TH_SET;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffc1))
            {
                PORT_Regulate_DATA.RMS_CURRENT_5TH_SET=0.5*40.0;
                PORT_Regulate_DATA.PHA_CURRENT_5TH_SET=0.0;
//              PORT_Regulate_DATA.PHA_CURRENT_5TH_SET=Phase_PLL;

                Har_5_RMS=PORT_Regulate_DATA.RMS_CURRENT_5TH_SET;
                Har_5_PH=PORT_Regulate_DATA.PHA_CURRENT_5TH_SET;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffc2))
            {
                PORT_Regulate_DATA.RMS_CURRENT_7TH_SET=0.5*40.0;
                PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=0.0;
//              PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=Phase_PLL;

                Har_7_RMS=PORT_Regulate_DATA.RMS_CURRENT_7TH_SET;
                Har_7_PH=PORT_Regulate_DATA.PHA_CURRENT_7TH_SET;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffc3))
            {
                PORT_Regulate_DATA.RMS_CURRENT_7TH_SET=0.5*30.0;
                PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=0.0;
//              PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=Phase_PLL;

                Har_7_RMS=PORT_Regulate_DATA.RMS_CURRENT_7TH_SET;
                Har_7_PH=PORT_Regulate_DATA.PHA_CURRENT_7TH_SET;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffc4))
            {
                PORT_Regulate_DATA.RMS_CURRENT_11TH_SET=0.02*40.0;
                PORT_Regulate_DATA.PHA_CURRENT_11TH_SET=0.0;
//              PORT_Regulate_DATA.PHA_CURRENT_11TH_SET=Phase_PLL;

                Har_11_RMS=PORT_Regulate_DATA.RMS_CURRENT_11TH_SET;
                Har_11_PH=PORT_Regulate_DATA.PHA_CURRENT_11TH_SET;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffd1))
            {
                PORT_Regulate_DATA.RMS_CURRENT_5TH_SET=0.1*40.0;
                PORT_Regulate_DATA.PHA_CURRENT_5TH_SET=0.0;
//              PORT_Regulate_DATA.PHA_CURRENT_5TH_SET=Phase_PLL;

                Har_5_RMS=PORT_Regulate_DATA.RMS_CURRENT_5TH_SET;
                Har_5_PH=PORT_Regulate_DATA.PHA_CURRENT_5TH_SET;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffd2))
            {
                PORT_Regulate_DATA.RMS_CURRENT_7TH_SET=0.1*40.0;
                PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=0.0;
//              PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=Phase_PLL;

                Har_7_RMS=PORT_Regulate_DATA.RMS_CURRENT_7TH_SET;
                Har_7_PH=PORT_Regulate_DATA.PHA_CURRENT_7TH_SET;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffd3))
            {
                PORT_Regulate_DATA.RMS_CURRENT_11TH_SET=0.05*40.0;
                PORT_Regulate_DATA.PHA_CURRENT_11TH_SET=0.0;
//              PORT_Regulate_DATA.PHA_CURRENT_11TH_SET=Phase_PLL;

                Har_11_RMS=PORT_Regulate_DATA.RMS_CURRENT_11TH_SET;
                Har_11_PH=PORT_Regulate_DATA.PHA_CURRENT_11TH_SET;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffE1))
            {
                PORT_Regulate_DATA.RMS_CURRENT_5TH_SET=0.2*40.0;
                PORT_Regulate_DATA.PHA_CURRENT_5TH_SET=0.0;
//              PORT_Regulate_DATA.PHA_CURRENT_5TH_SET=Phase_PLL;

                Har_5_RMS=PORT_Regulate_DATA.RMS_CURRENT_5TH_SET;
                Har_5_PH=PORT_Regulate_DATA.PHA_CURRENT_5TH_SET;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffE2))
            {
                PORT_Regulate_DATA.RMS_CURRENT_5TH_SET=0.1*40.0;
                PORT_Regulate_DATA.PHA_CURRENT_5TH_SET=0.0;
//              PORT_Regulate_DATA.PHA_CURRENT_5TH_SET=Phase_PLL;

                Har_5_RMS=PORT_Regulate_DATA.RMS_CURRENT_5TH_SET;
                Har_5_PH=PORT_Regulate_DATA.PHA_CURRENT_5TH_SET;

                PORT_Regulate_DATA.RMS_CURRENT_7TH_SET=0.05*40.0;
                PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=0.0;
//              PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=Phase_PLL;

                Har_7_RMS=PORT_Regulate_DATA.RMS_CURRENT_7TH_SET;
                Har_7_PH=PORT_Regulate_DATA.PHA_CURRENT_7TH_SET;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffE3))
            {
                PORT_Regulate_DATA.RMS_CURRENT_5TH_SET=0.2*40.0;
                PORT_Regulate_DATA.PHA_CURRENT_5TH_SET=0.0;
//              PORT_Regulate_DATA.PHA_CURRENT_5TH_SET=Phase_PLL;

                Har_5_RMS=PORT_Regulate_DATA.RMS_CURRENT_5TH_SET;
                Har_5_PH=PORT_Regulate_DATA.PHA_CURRENT_5TH_SET;

                PORT_Regulate_DATA.RMS_CURRENT_7TH_SET=0.1*40.0;
                PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=0.0;
//              PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=Phase_PLL;

                Har_7_RMS=PORT_Regulate_DATA.RMS_CURRENT_7TH_SET;
                Har_7_PH=PORT_Regulate_DATA.PHA_CURRENT_7TH_SET;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffe4))
            {
                PORT_Regulate_DATA.RMS_CURRENT_7TH_SET=0.2*40.0;
                PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=0.0;
//              PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=Phase_PLL;

                Har_7_RMS=PORT_Regulate_DATA.RMS_CURRENT_7TH_SET;
                Har_7_PH=PORT_Regulate_DATA.PHA_CURRENT_7TH_SET;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffE5))
            {
                PORT_Regulate_DATA.RMS_CURRENT_5TH_SET=0.2*30.0;
                PORT_Regulate_DATA.PHA_CURRENT_5TH_SET=0.0;
//              PORT_Regulate_DATA.PHA_CURRENT_5TH_SET=Phase_PLL;

                Har_5_RMS=PORT_Regulate_DATA.RMS_CURRENT_5TH_SET;
                Har_5_PH=PORT_Regulate_DATA.PHA_CURRENT_5TH_SET;
            }
            else if((Addr_write_temp==0x60000620)&&(WriteD==0xffe6))
            {
                PORT_Regulate_DATA.RMS_CURRENT_7TH_SET=0.2*30.0;
                PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=0.0;
//              PORT_Regulate_DATA.PHA_CURRENT_7TH_SET=Phase_PLL;

                Har_7_RMS=PORT_Regulate_DATA.RMS_CURRENT_7TH_SET;
                Har_7_PH=PORT_Regulate_DATA.PHA_CURRENT_7TH_SET;
            }
            else
            {
//              PWMPluse_Disable(WriteD&0x00ff);
            }
*/


            HWREGH(Addr_write_temp)=WriteD;
            if(WriteD==HWREGH(Addr_write_temp))
            {
                WriteOK=0x28;
            }
            else
            {
                WriteOK=0x19;
            }

            /* 旧版上位机通讯调用已废弃 */
        // Uart2_send(WriteController);
        }
        else
        {
            for(i=0;i<ReadLength;i++)
            {
                WriteD=HWREGH(Addr_write_temp);
                Addr_write_temp=Addr_write_temp+2;

                i_temp=(unsigned char)(i<<1);
                PCuartBUFF[i_temp+8]=WriteD;
                PCuartBUFF[i_temp+9]=WriteD>>8;
            }
            /* 旧版上位机通讯调用已废弃 */
        // Uart2_send(ReadController);
        }
    }
}
#endif
#endif


/****************************************************************************/
/*                                                                          */
/*              CRC                                                         */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 计算旧 UART 兼容帧的 CRC16，保持原多项式及交换字节规则
 * 输入参数: ba：CRC 输入缓冲区首地址，调用者提供有效帧字节
 *           size：输入缓存有效长度
 * 输出参数: 无指针输出；通过返回值提供计算结果或状态。
 * 返回值: 原实现生成的 16 位 CRC 值
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
unsigned short CRC16(unsigned char *ba, int size)
{
    CRC crc; /* crc：逐字节、逐位累加的 CRC 校验状态。 */
    crc.crc16 = 0xffff;
    int i, l; /* i：本次字节/位循环序号。 */
    /* l：CRC 当前位循环序号。 */

    for (i=0; i<size; i++)
    {
        unsigned char ch = ba[i]; /* ch：当前参与 CRC 的输入字节。 */
        crc.by[0] = crc.by[0] ^ ch;
        for (l=0; l<8; l++)
        {
            if (crc.by[0] & 0x01)
            {
                crc.crc16 = crc.crc16 >> 1;
                crc.crc16 = crc.crc16 ^ 0xa001;
            }
            else
            {
                crc.crc16 = crc.crc16 >> 1;
            }
        }
    }
    unsigned char swap = crc.by[0]; /* swap：原 CRC 高低字节交换的中间值。 */
    crc.by[0] = crc.by[1];
    crc.by[1] = swap;

    return crc.crc16;
}

/****************************************************************************/
/*                                                                          */
/*              keyscan                                                     */
/*                                                                          */
/****************************************************************************/
/*==================================================================
 * 函数功能: 保留原本地按键扫描入口及按键状态处理
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void keyscan(void)
{
//  ushort keytt;
    if(GPIOPinRead(SOC_GPIO_0_REGS, 67)==0)
    {
        Delay(0xffff);
        Delay(0xffff);

        if(GPIOPinRead(SOC_GPIO_0_REGS, 67)==0)
        {
            GPIOPinWrite(SOC_GPIO_0_REGS, 3, 1);
            while(GPIOPinRead(SOC_GPIO_0_REGS, 67)==0);
            GPIOPinWrite(SOC_GPIO_0_REGS, 3, 0);

            Flag_Key=1;
        }
    }

    if(Flag_Key==1)
    {
        Flag_Key=0;


//      keytt=FPGA_Interface[2].ID.ID16;
        GPIOPinWrite(SOC_GPIO_0_REGS, 104, 1);
//      UARTCharPut(SOC_UART_2_REGS, 0xeb);
//      UARTCharPut(SOC_UART_2_REGS, keytt>>8);
//      UARTCharPut(SOC_UART_2_REGS, keytt);
//      UARTCharPut(SOC_UART_2_REGS, 0x90);
        Delay(0xffff);
        GPIOPinWrite(SOC_GPIO_0_REGS, 104, 0);

/*
        RUN_Status=0;
        ((unsigned short *)SOC_EMIFA_CS2_ADDR)[DABRESET_Addr]= 0xeb;
        Flag_400ms=1;
        while(Flag_400ms);
        ((unsigned short *)SOC_EMIFA_CS2_ADDR)[Port_RESET_Addr]= 0xeb;  //SO RESET


        Flag_Fault=0;

        Delay(0xffff);

        if(Flag_SystemWork==0)
        {
            Flag_SystemWork=0xeb;

                Vd_Out_DCAC=0.0;
//          Flag_SystemWork=0;
                //////////////////////////////////////////
//          Flag_StartACDCOver=0xeb;
//          Work_State=0xc0;
        }
        else
        {
            Flag_SystemWork=0;
        }

        SwitchOut=0xffff&SwitchMask;
        ((unsigned short *)SOC_EMIFA_CS2_ADDR)[SwitchOut_Addr]= SwitchOut;
*/
    }
}


/****************************************************************************/
/*                                                                          */
/*              UART INT routine                                           */
/*                                                                          */
/****************************************************************************/
volatile unsigned char UartDebugBuf[32]; /* UartDebugBuf：原 UART 调试发送缓存。 */
volatile unsigned int UartDebugIndex; /* UartDebugIndex：原 UART 调试缓存当前位置。 */
/*==================================================================
 * 函数功能: 保留原 UART2 ISR 符号；调试通信主流程不注册此中断
 * 输入参数: 无
 * 输出参数: 板级寄存器、控制/采样全局量及相应后台任务状态。
 * 返回值: 无
 * 执行说明: 入口、主循环或原中断路径；中断正文的可执行代码保持注释前版本。
 *================================================================*/
void UART2Isr(void)
{
    unsigned char rxData = 0; /* rxData：原 UART ISR 读取的接收字符。 */
    unsigned int int_id = 0; /* int_id：原 UART 中断源标识。 */
        unsigned int rxtemp; /* rxtemp：原 UART 接收处理临时字符。 */

    int_id = UARTIntStatus(SOC_UART_2_REGS);

    IntEventClear(SYS_INT_UART2_INT);

    if(UART_INTID_RX_DATA == int_id)
    {
        rxData = UARTCharGetNonBlocking(SOC_UART_2_REGS); // 每次中断只取一个字节

        /* A5 5A维护帧优先交给新协议；普通地址帧继续走原485解析器。 */
        if(MaintenanceProtocol_RxByteForUart(rxData))
        {
            if(UartDebugIndex < 32)
                UartDebugBuf[UartDebugIndex++] = rxData;
            return;
        }
        if(UartDebugIndex < 32) {
            UartDebugBuf[UartDebugIndex++] = rxData;
        }

                CommNum++;
                if(CommNum>8)
                {
                    CommNum=9;
                }

                /* 旧协议固定按：地址、功能、32位地址、长度、数据接收。 */
                switch(CommNum)
                {
                    case 1:
                    {
                        if(rxData!=ControllerAddr)
                        {
                            CommNum=0;
                        }
                        else
                        {
                            ReceiveuartBUFF[0]=rxData;
                        }
                    }break;
                    case 2:
                    {
                        if((rxData!=ReadController)&&(rxData!=WriteController))
                        {
                            CommNum=0;
                        }
                        else
                        {
                            CommFunc=rxData;
                            ReceiveuartBUFF[1]=rxData;
                        }
                    }break;
                case 3:
                    {
                        StartAddr=rxData;
                        ReceiveuartBUFF[2]=rxData;
                    }break;
                case 4:
                    {
                        rxtemp=(unsigned int)rxData;
                        rxtemp=rxtemp<<8;
                        StartAddr=StartAddr|rxtemp;

                        ReceiveuartBUFF[3]=rxData;
                    }break;
                case 5:
                    {
                        rxtemp=(unsigned int)rxData;
                        rxtemp=rxtemp<<16;
                        StartAddr=StartAddr|rxtemp;

                        ReceiveuartBUFF[4]=rxData;
                    }break;
                case 6:
                    {
                        rxtemp=(unsigned int)rxData;
                        rxtemp=rxtemp<<24;
                        StartAddr=StartAddr|rxtemp;

                        ReceiveuartBUFF[5]=rxData;
                    }break;
                    case 7:
                    {
                        ReceiveuartBUFF[6]=rxData;
                    }break;
                    case 8:
                    {
                        ReceiveuartBUFF[7]=rxData;

                        if(CommFunc==ReadController)
                        {
                            ReadLength=ReceiveuartBUFF[6];
                            ReadLength=ReadLength<<8;
                            ReadLength=ReadLength+ReceiveuartBUFF[7];
                        }

                        CommLength=2;
                        CommInNum=0;
                    }break;
                    case 9:
                    {
                        ReceiveuartBUFF[8+CommInNum++]=rxData;
                        if(CommInNum==CommLength)
                        {
                            /* 收齐数据后只置标志，完整解析放到主循环，避免ISR过长。 */
                            CommTotalNum=6+CommLength;
                            Flag_Uart=1;
                            UARTIntDisable(SOC_UART_2_REGS, UART_INT_RXDATA_CTI);
                            CommInNum=0;
                            CommNum=0;
                            CommLength=0;
                        }
                    }break;
                }
    }
    return;
}

