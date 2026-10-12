/* Shared fake UART/GPIO/EEPROM declarations for independently compiled modules.
 * This header is force-included only in temporary host regression units. */
#ifndef SST_HOST_PROTOCOL_IO_H
#define SST_HOST_PROTOCOL_IO_H
#include <stdint.h>
#include "ChbDabControl.h"
#include "ChbBuildConfig.h"
#include "SstFault.h"
#include "SstDebug.h"
#include "SstParams.h"
#include "SstBoard.h"
#include "CpuTask.h"
#define SOC_UART_2_REGS 0U
#define SOC_GPIO_0_REGS 0U
#define UART_LSR 0U
#define UART_LSR_TEMT 0x40U
#define UART_INT_RXDATA_CTI 1U
#define UART_INT_TX_EMPTY 2U
#define UART_WORDL_8BITS 3U
#define UART_OVER_SAMP_RATE_16 4U
#define UART_RX_TRIG_LEVEL_1 1U
#define VALIDFLAG 1U
extern unsigned hwLsr;
#define HWREG(x) hwLsr
typedef struct { float PORT_V_A; } HOST_PORT_REALTIME;
extern HOST_PORT_REALTIME PORT_RealTime_DATA;
extern float HA_Port_Volt_A, HA_Port_Volt_B, HA_Port_Volt_C;
extern float HA_Port_Current_A, HA_Port_Current_B, HA_Port_Current_C;
extern unsigned short Port_State;
void UARTIntDisable(unsigned,unsigned);
void UARTConfigSetExpClk(unsigned,unsigned,unsigned,unsigned,unsigned);
void UARTFIFOEnable(unsigned);
void UARTFIFOLevelSet(unsigned,unsigned);
void UARTEnable(unsigned);
void GPIOPinWrite(unsigned,unsigned,unsigned);
unsigned UARTCharsAvail(unsigned);
unsigned UARTCharGetNonBlocking(unsigned);
unsigned UARTSpaceAvail(unsigned);
void UARTCharPutNonBlocking(unsigned,unsigned);
uint8_t r_eeprom_byte_read(uint8_t,uint8_t);
uint8_t r_eeprom_byte_write(uint8_t,uint8_t,uint8_t);
#endif
