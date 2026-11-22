#ifndef SST_FAULT_SUMMARY_H
#define SST_FAULT_SUMMARY_H

#include <stdint.h>
#include "ChbDabControl.h"

/* 原端口字 bit13/14 复用为软件 OV/OC，外部摘要的 bit13/14 则为通信和同步。 */
static inline uint16_t SstFault_EncodeSummary(uint16_t port, uint16_t type,
    uint16_t software_oc, uint16_t software_ov, uint16_t core_fault, uint16_t state)
{
    uint16_t fault = port & 0x1FFFU;
    if (type & 0x0100U)
        fault |= 0x0001U;
    if (type & 0x0200U)
        fault |= 0x0002U;
    if (type & 0x0400U)
        fault |= 0x0008U;
    if (type & 0xE0F0U)
        fault |= 0x0040U;
    if (type & 0x000CU)
        fault |= 0x0080U;
    if ((port & 0x4000U) || (type & 0x0002U) || software_oc ||
        core_fault == CHB_FAULT_AC_OVERCURRENT)
        fault |= 0x0400U;
    if (type & 0x4000U)
        fault |= 0x0800U;
    if ((port & 0x2000U) || (type & 0x0001U) || software_ov ||
        core_fault == CHB_FAULT_DC_OVERVOLTAGE)
        fault |= 0x1000U;
    if (type & 0x1000U)
        fault |= 0x2000U;
    if (type & 0x0800U)
        fault |= 0x4000U;
    if (fault || port || type || core_fault != CHB_FAULT_NONE ||
        state == CHB_STATE_FAULT)
        fault |= 0x8000U;
    return fault;
}

#endif
