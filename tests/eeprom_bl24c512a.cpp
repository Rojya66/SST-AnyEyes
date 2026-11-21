/* Execute the production bit-banged driver against an edge-level open-drain
 * I2C slave. The simulated device deliberately wraps writes inside 128 bytes. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include "EEProm.H"
#include "gpio.h"

enum BusPhase { IDLE, RECEIVE, RECEIVE_ACK_SETUP, SLAVE_ACK, TRANSMIT, MASTER_ACK, END_READ };
static BusPhase phase = IDLE;
static bool sdaOutput, sclOutput, slaveLow, missing, stuckSda, stuckScl;
static unsigned bitCount, received, ordinal, busyPolls, forcedBusy, nackOrdinal = 999;
static unsigned starts, stops, repeatedStarts;
static bool accepted, readDirection, masterMore;
static uint16_t pointerAddress, pageStart;
static uint8_t deviceMemory[65536];
static std::vector<uint8_t> transaction, pending, readAcks;
static std::vector<std::vector<uint8_t> > transactions;

static bool Sda(void) {return (sdaOutput == false) && (slaveLow == false) && (stuckSda == false);}
static bool Scl(void) {return (sclOutput == false) && (stuckScl == false);}
static void SetTransmitBit(void)
{slaveLow = (deviceMemory[pointerAddress] & (0x80U >> bitCount)) == 0U;}

static bool ReceiveByte(uint8_t value)
{
    transaction.push_back(value);
    unsigned position = ordinal++;
    if (position == nackOrdinal)
        return false;
    if (position == 0U)
    {
        if (missing || forcedBusy != 0U || busyPolls != 0U)
        {
            if (busyPolls != 0U) --busyPolls;
            return false;
        }
        if ((value & 0xFEU) != SST_EEPROM_DEVICE_WRITE)
            return false;
        readDirection = (value & 1U) != 0U;
    }
    else if (position == 1U)
        pointerAddress = (uint16_t)value << 8U;
    else if (position == 2U)
    {
        pointerAddress = (uint16_t)(pointerAddress | value);
        pageStart = pointerAddress;
    }
    else
        pending.push_back(value);
    return true;
}

static void RisingEdge(void)
{
    if (phase == RECEIVE)
    {
        received = (received << 1U) | (Sda() ? 1U : 0U);
        if (++bitCount == 8U)
        {
            accepted = ReceiveByte((uint8_t)received);
            phase = RECEIVE_ACK_SETUP;
        }
    }
    else if (phase == TRANSMIT)
    {
        ++bitCount;
    }
    else if (phase == MASTER_ACK)
    {
        masterMore = (Sda() == false);
        readAcks.push_back(masterMore ? 0U : 1U);
    }
}

static void FallingEdge(void)
{
    if (phase == RECEIVE_ACK_SETUP)
    {
        slaveLow = accepted;
        phase = SLAVE_ACK;
    }
    else if (phase == SLAVE_ACK)
    {
        slaveLow = false;
        bitCount = received = 0U;
        if (accepted == false)
            phase = END_READ;
        else if (readDirection)
        {
            phase = TRANSMIT;
            SetTransmitBit();
        }
        else
            phase = RECEIVE;
    }
    else if (phase == TRANSMIT)
    {
        if (bitCount < 8U)
            SetTransmitBit();
        else
        {
            slaveLow = false;
            phase = MASTER_ACK;
        }
    }
    else if (phase == MASTER_ACK)
    {
        pointerAddress = (uint16_t)(pointerAddress + 1U);
        bitCount = 0U;
        phase = masterMore ? TRANSMIT : END_READ;
        if (masterMore) SetTransmitBit();
    }
}

void GPIOPinWrite(unsigned, unsigned pin, unsigned value)
{
    assert(pin == SST_EEPROM_SDA_PIN || pin == SST_EEPROM_SCL_PIN);
    assert(value == 0U); // High must always release, never push-pull drive.
}
void GPIODirModeSet(unsigned, unsigned pin, unsigned mode)
{
    bool oldSda = Sda(), oldScl = Scl();
    assert(mode == GPIO_DIR_INPUT || mode == GPIO_DIR_OUTPUT);
    if (pin == SST_EEPROM_SDA_PIN) sdaOutput = mode == GPIO_DIR_OUTPUT;
    else {assert(pin == SST_EEPROM_SCL_PIN); sclOutput = mode == GPIO_DIR_OUTPUT;}
    bool newSda = Sda(), newScl = Scl();
    if (oldScl && newScl && oldSda && (newSda == false))
    {
        if (phase != IDLE) {++repeatedStarts; transactions.push_back(transaction);}
        ++starts; phase = RECEIVE; slaveLow = false;
        bitCount = received = ordinal = 0U; readDirection = false;
        transaction.clear(); pending.clear();
    }
    else if (oldScl && newScl && (oldSda == false) && newSda)
    {
        ++stops; transactions.push_back(transaction);
        for (unsigned i=0U; i<pending.size(); ++i)
            deviceMemory[(pageStart & 0xFF80U) | ((pageStart+i) & 0x7FU)] = pending[i];
        if (pending.empty() == false) busyPolls = 2U;
        phase = IDLE; slaveLow = false; transaction.clear(); pending.clear();
    }
    else if ((oldScl == false) && newScl) RisingEdge();
    else if (oldScl && (newScl == false)) FallingEdge();
}
int GPIOPinRead(unsigned, unsigned pin)
{return (pin == SST_EEPROM_SDA_PIN ? Sda() : Scl()) ? 1 : 0;}

/* DRIVER */

static void ResetBus(void)
{
    phase=IDLE;sdaOutput=sclOutput=slaveLow=missing=stuckSda=stuckScl=false;
    busyPolls=forcedBusy=0U;nackOrdinal=999U;starts=stops=repeatedStarts=0U;
    transactions.clear();transaction.clear();pending.clear();readAcks.clear();
    Eeprom_Initialize();assert(Sda()&&Scl());
}
int main(void)
{
    uint8_t data[10], back[10];
    memset(deviceMemory,0xFF,sizeof(deviceMemory));ResetBus();
    for (unsigned i=0U;i<10U;++i) data[i]=(uint8_t)(0x91U+i);
    assert(Eeprom_Write(0x007DU,data,10U)==VALIDFLAG);
    assert(memcmp(deviceMemory+0x7D,data,10U)==0);
    unsigned dataWrites=0U;
    for (const auto &t:transactions)
    {
        if (t.size()>3U)
        {
            if (dataWrites++==0U) assert(t.size()==6U&&t[0]==0xA0U&&t[1]==0U&&t[2]==0x7DU);
            else assert(t.size()==10U&&t[1]==0U&&t[2]==0x80U);
        }
    }
    assert(dataWrites==2U&&g_sst_eeprom_status.error==0U);
    transactions.clear();readAcks.clear();
    assert(Eeprom_Read(0x007DU,back,10U)==VALIDFLAG&&memcmp(back,data,10U)==0);
    assert(repeatedStarts==1U&&readAcks.size()==10U&&readAcks.back()==1U);
    for (unsigned i=0U;i<9U;++i) assert(readAcks[i]==0U);
    assert(transactions[0]==std::vector<uint8_t>({0xA0U,0U,0x7DU}));
    assert(transactions[1]==std::vector<uint8_t>({0xA1U}));

    ResetBus();assert(r_eeprom_byte_write(6U,3U,0x5AU)==VALIDFLAG);
    assert(deviceMemory[0x0183U]==0x5AU); // Logical page remains 64 bytes.
    busyPolls=0U;assert(r_eeprom_section_read(6U,3U,1U,back)==VALIDFLAG&&back[0]==0x5AU);
    assert(Eeprom_Write(0xFFFEU,data,2U)==VALIDFLAG);
    assert(Eeprom_Read(0xFFFEU,back,2U)==VALIDFLAG&&memcmp(back,data,2U)==0);
    unsigned oldStarts=starts;
    assert(Eeprom_Read(0xFFFFU,back,2U)==INVALIDFLAG&&g_sst_eeprom_status.error==EEPROM_ERROR_ADDRESS);
    assert(starts==oldStarts);

    ResetBus();missing=true;
    assert(Eeprom_Read(0x0180U,back,1U)==INVALIDFLAG);
    assert(starts==1U&&stops==1U&&Sda()&&Scl());
    assert(g_sst_eeprom_status.error==EEPROM_ERROR_NO_ACK&&g_sst_eeprom_status.stage==1U);
    ResetBus();nackOrdinal=2U;
    assert(Eeprom_StartByteWrite(0x1234U,0x11U)==INVALIDFLAG);
    assert(g_sst_eeprom_status.stage==3U&&transactions.back().size()==3U&&Sda()&&Scl());
    ResetBus();forcedBusy=1U;
    assert(Eeprom_Write(0U,data,1U)==INVALIDFLAG);
    assert(g_sst_eeprom_status.error==EEPROM_ERROR_WRITE_TIMEOUT);
    assert(starts==SST_EEPROM_READY_POLLS&&Sda()&&Scl());
    ResetBus();stuckSda=true;
    assert(Eeprom_Read(0U,back,1U)==INVALIDFLAG&&g_sst_eeprom_status.error==EEPROM_ERROR_BUS_LOW);
    puts("PASS: real BL24C512A driver; A0/A1 and 16-bit address; open-drain edges; page split; 64-byte logical mapping; final NACK; bounded busy; NACK STOP and bus-low diagnostics");
}
