/*
 * CometOS --- a component-based, extensible, tiny operating system
 *             for wireless networks
 *
 * Copyright (c) 2015, Institute of Telematics, Hamburg University of Technology
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "CC110lApi.h"

#include <avr/pgmspace.h>

namespace cometos {

/* Radio configurations exported from SmartRF Studio*/
static const ccRegisterSetting_t cc110LDataRateRfSettings[] PROGMEM = {
        {CC110L_ADDR            ,0x00}, /* ADDR          Device Address                                 */
        {CC110L_CHANNR          ,0x00}, /* CHANNR        Frequency channel default 0x00                 */
        {CC110L_FSCTRL1         ,0x06}, /* FSCTRL1       Frequency Synthesizer Control                  */
        {CC110L_FSCTRL0         ,0x00}, /* FSCTRL0       Frequency Synthesizer Control                  */
        {CC110L_MDMCFG4         ,0x84}, /*(64?) MDMCFG4       Modem Configuration                        */
        {CC110L_MDMCFG3         ,0x75}, /*(83) MDMCFG3       Modem Configuration                        */
        {CC110L_MDMCFG2         ,0x0B}, /* MDMCFG2       Modem Configuration                            */
        {CC110L_MDMCFG1         ,0x23}, /* MDMCFG1       Modem Configuration                            */
        {CC110L_MDMCFG0         ,0x2F}, /* MDMCFG0       Modem Configuration                            */
        {CC110L_DEVIATN         ,0x47}, /* DEVIATN       Modem Deviation Setting                        */
        {CC110L_MCSM2           ,0x07}, /* MCSM2         Main Radio Control State Machine Configuration */
        {CC110L_FOCCFG          ,0x16}, /* FOCCFG        Frequency Offset Compensation Configuration    */
        {CC110L_BSCFG           ,0x6C}, /* BSCFG         Bit Synchronization Configuration              */
        {CC110L_AGCCTRL2        ,0x03}, /* AGCCTRL2      AGC Control                                    */
        {CC110L_AGCCTRL1        ,0x40}, /* AGCCTRL1      AGC Control                                    */
        {CC110L_AGCCTRL0        ,0x91}, /* AGCCTRL0      AGC Control                                    */
        {CC110L_RESERVED_0X20   ,0xFB}, /*RESERVED_0X20    Reserved register                      */
        {CC110L_FREND1          ,0x56}, /* FREND1        Front End RX Configuration                     */
        {CC110L_FREND0          ,0x10}, /* FREND0        Front End TX Configuration                     */
        {CC110L_TEST2           ,0x81}, /* given by SmartRFStudio TEST2         Various Test Settings                          */
        {CC110L_TEST1           ,0x35}, /* given by SmartRFStudio TEST1         Various Test Settings                          */
        {CC110L_TEST0           ,0x09}  /* given by SmartRFStudio TEST0         Various Test Settings                          */
};

/* Common register settings for CC11xL radios and test case*/
// TODO this is not possible in PROGMEM, why?
static const ccRegisterSetting_t commonRfSettings[] PROGMEM = {
    //  {CC110L_IOCFG2          ,0x29}, /* IOCFG2          GDO2 Output Pin Configuration (CHIP_RDYn)         */
        {CC110L_IOCFG2          ,0x1B}, /* IOCFG2          GDO2 Output Pin Configuration ()         */
        {CC110L_IOCFG1          ,0x2E}, /* IOCFG1          GDO1 Output Pin Configuration (high impedance)    */
        {CC110L_IOCFG0          ,0x06}, /* IOCFG0          GDO0 Output Pin Configuration
        (Asserts when sync word has been sent / received, and de-asserts at the end of the packet. In RX, the
        pin will also de-assert when a packet is discarded due to address or maximum length filtering or when
        the radio enters RXFIFO_OVERFLOW state. In TX the pin will de-assert if the TX FIFO underflows.     */
//      {CC110L_FIFOTHR         ,0xCC}, /* FIFOTHR         RX FIFO and TX FIFO Thresholds                    */
        {CC110L_FIFOTHR         ,0xC7}, /* FIFOTHR         RX FIFO and TX FIFO Thresholds                    */
        {CC110L_SYNC1           ,0xD3}, /* SYNC1           Sync Word, High Byte                              */
        {CC110L_SYNC0           ,0x91}, /* SYNC0           Sync Word, Low Byte                               */
        {CC110L_PKTLEN          ,0xFF}, /* PKTLEN          Packet Length                                     */
        {CC110L_PKTCTRL1        ,0x04},
        {CC110L_PKTCTRL0        ,0x05}, /* PKTCTRL0        Packet Automation Control                         */
        {CC110L_FREQ2           ,0x20}, /* FREQ2           Frequency Control Word, High Byte                 */
        {CC110L_FREQ1           ,0x34}, /* FREQ1           Frequency Control Word, Middle Byte               */
        {CC110L_FREQ0           ,0x62}, /* FREQ0           Frequency Control Word, Low Byte                  */
        {CC110L_MCSM1           ,0x00}, /* MCSM1           Main Radio Control State Machine Configuration    */
        {CC110L_MCSM0           ,0x18}, /* MCSM0           Main Radio Control State Machine Configuration    */
        {CC110L_FSCAL3          ,0xE9}, /* given by SmartRFStudio (not a9) FSCAL3          Frequency Synthesizer Calibration                 */
        {CC110L_FSCAL2          ,0x2A}, /* given by SmartRFStudio (not a)  FSCAL2 Frequency Synthesizer Calibration                 */
        {CC110L_FSCAL1          ,0x00}, /* given by SmartRFStudio (not 20) FSCAL1      Frequency Synthesizer Calibration                 */
        {CC110L_FSCAL0          ,0x1F}, /* given by SmartRFStudio (not d)  FSCAL0         Frequency Synthesizer Calibration                 */
        {CC110L_RESERVED_0X29   ,0x89}, /* RESERVED_0X29   Reserved register                                 */
        {CC110L_RESERVED_0X2A   ,0x7F}, /* RESERVED_0X2A   Reserved register                                 */
        {CC110L_RESERVED_0X2B   ,0x63}, /* RESERVED_0X2B   Reserved register                                 */
};

#define FXOSC_MHZ (27)

ccRegisterSetting_t getRegisterSetting(PGM_P address_short, uint8_t num) {
        ccRegisterSetting_t r;
        r.addr = pgm_read_word_near(address_short+sizeof(r)*num);
        r.data = pgm_read_word_near(address_short+sizeof(r)*num+sizeof(r.addr));
        return r;
}

void CC110lApi::regConfig()
{
    /* initialize radio registers given the selected cc110LSettings */
    uint8_t data;

    /* Log that radio is in IDLE state */
    rxIdle();
    changeState(CC110L_STATE_IDLE);

    for(uint16_t i = 0; i < (sizeof cc110LDataRateRfSettings/sizeof(ccRegisterSetting_t));i++)
    {
        auto regset = getRegisterSetting((PGM_P)cc110LDataRateRfSettings, i);
        data = regset.data;
        writeAccess(regset.addr,&data,1);
    }

    /* Common settings for PER test regardless of radio configuration */
    for(uint16_t i = 0; i < (sizeof commonRfSettings/sizeof(ccRegisterSetting_t));i++)
    {
        auto regset = getRegisterSetting((PGM_P)commonRfSettings, i);
        data = regset.data;
        writeAccess(regset.addr,&data,1);
    }

    return;
}

void CC110lApi::dumpSettings(void)
{
    uint8_t data;

    for(uint16_t i = 0; i < (sizeof cc110LDataRateRfSettings/sizeof(ccRegisterSetting_t));i++)
    {
        auto regset = getRegisterSetting((PGM_P)cc110LDataRateRfSettings, i);
        readAccess(regset.addr,&data,1);
        cometos::getCout() << cometos::hex << (int)regset.addr << " " << ((int)data) << cometos::endl;
    }

    for(uint16_t i = 0; i < (sizeof commonRfSettings/sizeof(ccRegisterSetting_t));i++)
    {
        auto regset = getRegisterSetting((PGM_P)commonRfSettings, i);
        readAccess(regset.addr,&data,1);
        cometos::getCout() << cometos::hex << (int)regset.addr << " " << ((int)data) << cometos::endl;
    }
}

// Output power settings for 868 MHz
#define CC110L_MIN_OUTPUT_POWER -33
#define CC110L_MAX_OUTPUT_POWER 10
uint8_t getPowerRegisterValue(int8_t power) {
    int32_t p = power;
    if(p < -33) {
        return 0;
    }
    else if(p <= -20) {
        return 48*p*p/1000 + 36*p/10 + 67;
    }
    else if(p <= -15) {
        return 78*p*p/1000 + 41*p/10 + 74;
    }
    else if(p <= -5) {
        return 71*p*p/1000 + 24*p/10 + 56;
    }
    else if(p <= -3) {
        return 57*p*p/100 + 66*p/10 + 76;
    }
    else if(p <= -1) {
        return -15*p/10 + 80;
    }
    else if(p <= 5) {
        return -32*p*p/100 - 87*p/100 + 142;
    }
    else if(p <= 10) {
        return -22*p*p/1000 - 25*p/10 + 221;
    }
    else {
        return 0xC2;
    }
}

int8_t CC110lApi::getMinUnamplifiedOutputPower() {
    return CC110L_MIN_OUTPUT_POWER;
}

int8_t CC110lApi::getMaxUnamplifiedOutputPower() {
    return CC110L_MAX_OUTPUT_POWER;
}

cometos_error_t CC110lApi::setUnamplifiedOutputPower(int8_t powerdBm) {
    if(powerdBm > getMaxUnamplifiedOutputPower()) {
        return COMETOS_ERROR_FAIL;
    }

    if(powerdBm < getMinUnamplifiedOutputPower()) {
        return COMETOS_ERROR_FAIL;
    }

    uint8_t level = getPowerRegisterValue(powerdBm);
    writeAccess(CC110L_PA_TABLE0,&level,1);
    return COMETOS_SUCCESS;
}

cometos_error_t CC110lApi::setFrequency(uint32_t freqKHZ)
{
    uint32_t FREQ = ((((uint64_t)freqKHZ)*1000)<<(10))/(FXOSC_MHZ*(uint32_t)(1000000>>6));
    uint8_t FREQ2 = (FREQ >> 16) & 0xFF;
    uint8_t FREQ1 = (FREQ >> 8) & 0xFF;
    uint8_t FREQ0 = (FREQ) & 0xFF;
    writeAccess(CC110L_FREQ2,&FREQ2,1);
    writeAccess(CC110L_FREQ1,&FREQ1,1);
    writeAccess(CC110L_FREQ0,&FREQ0,1);

    return COMETOS_SUCCESS; // TODO error handling
}

cometos_error_t CC110lApi::setDataRate(uint32_t datarateBaud)
{
    uint8_t mdmcfg3;
    uint8_t mdmcfg4;
    readAccess(CC110L_MDMCFG4,&mdmcfg4,1); // other values should stay equal

    // calculation
    uint32_t inner = ((datarateBaud/10)<<(20-6))/(FXOSC_MHZ*(uint32_t)(1000000>>6)/10);

    uint8_t DRATE_E = 0;
    while(inner >>= 1) {
        DRATE_E++;
    }

    uint8_t exp = 28-DRATE_E;
    uint16_t DRATE_M = (datarateBaud<<(exp-6+1))/(FXOSC_MHZ*(uint32_t)(1000000>>(6)));
    // round to nearest
    if(DRATE_M & 0x1) {
        DRATE_M = ((DRATE_M >> 1) + 1) - 256;
    }
    else {
        DRATE_M = (DRATE_M >> 1) - 256;
    }

    if(DRATE_M == 256) {
        DRATE_E++;
        DRATE_M = 0;
    }

    mdmcfg4 &= ~(0xF);
    mdmcfg4 |= DRATE_E;
    mdmcfg3 = DRATE_M;

    writeAccess(CC110L_MDMCFG3,&mdmcfg3,1);
    writeAccess(CC110L_MDMCFG4,&mdmcfg4,1);

    // TODO error handling
    return COMETOS_SUCCESS;
}

cometos_error_t CC110lApi::setDeviation(uint32_t deviationHertz)
{
    uint32_t inner = ((deviationHertz)<<(17-3-6))/(FXOSC_MHZ*(uint32_t)(1000000>>6));

    uint8_t DEVIATION_E = 0;
    while(inner >>= 1) {
        DEVIATION_E++;
    }

    uint8_t exp = 17-DEVIATION_E;
    uint16_t DEVIATION_M = (deviationHertz<<(exp-6+1))/(FXOSC_MHZ*(uint32_t)(1000000>>(6)));

    // round to nearest
    if(DEVIATION_M & 0x1) {
        DEVIATION_M = ((DEVIATION_M >> 1) + 1) - 8;
        if(DEVIATION_M == 8) {
            DEVIATION_E++;
            DEVIATION_M = 0;
        }
    }
    else {
        DEVIATION_M = (DEVIATION_M >> 1) - 8;
    }

    uint8_t deviatn = DEVIATION_M | (DEVIATION_E << 4);

    writeAccess(CC110L_DEVIATN,&deviatn,1);

    // TODO error handling
    return COMETOS_SUCCESS;
}

cometos_error_t CC110lApi::setChannelSpacing(uint32_t channelSpacingHertz)
{
    uint8_t mdmcfg0;
    uint8_t mdmcfg1;
    readAccess(CC110L_MDMCFG1,&mdmcfg1,1); // other values should stay equal

    // calculation
    uint32_t inner = ((channelSpacingHertz)<<(16+2-8-6))/(FXOSC_MHZ*(uint32_t)(1000000>>6));

    uint8_t CHANSPC_E = 0;
    while(inner >>= 1) {
        CHANSPC_E++;
    }

    if(CHANSPC_E > 3) {
        return COMETOS_ERROR_INVALID;
    }

    uint8_t exp = 16+2-CHANSPC_E;
    uint8_t CHANSPC_M = (channelSpacingHertz/10<<(exp-6))/(FXOSC_MHZ*(uint32_t)(1000000>>6)/10)-256;

    mdmcfg1 &= ~(0x3);
    mdmcfg1 |= CHANSPC_E;
    mdmcfg0 = CHANSPC_M;

    writeAccess(CC110L_MDMCFG1,&mdmcfg1,1);
    writeAccess(CC110L_MDMCFG0,&mdmcfg0,1);

    return COMETOS_SUCCESS;
}

cometos_error_t CC110lApi::setChannel(uint8_t channel) {
    palExec_atomicBegin();

    rxIdle(); // TODO necessary?

    this->channel = channel;
    writeAccess(CC110L_CHANNR,&channel,1);
    palExec_atomicEnd();
    return COMETOS_SUCCESS;
}

uint8_t CC110lApi::getChannel() {
    return channel;
}

cometos_error_t CC110lApi::setChannelBandwidth(uint32_t bandwidthHertz)
{
    uint8_t mdmcfg4;
    readAccess(CC110L_MDMCFG4,&mdmcfg4,1); // other values should stay equal

    // log2(f/(BW*2^5))
    uint32_t inner = (FXOSC_MHZ*(uint32_t)(1000000>>5))/(bandwidthHertz);
    uint8_t CHANBW_E = 0;
    while(inner >>= 1) {
        CHANBW_E++;
    }

    uint8_t exp = 3+CHANBW_E;
    uint8_t CHANBW_M = (FXOSC_MHZ*(uint32_t)(1000000>>(3-1)))/(bandwidthHertz<<(exp-3));

    // round to nearest
    if(CHANBW_M & 0x1) {
        CHANBW_M = ((CHANBW_M >> 1) + 1) - 4;
        if(CHANBW_M == 4) {
            CHANBW_E++;
            CHANBW_M = 0;
        }
    }
    else {
        CHANBW_M = (CHANBW_M >> 1) - 4;
    }

    if(CHANBW_M > 3 || CHANBW_E > 3) {
        return COMETOS_ERROR_INVALID;
    }

    mdmcfg4 &= ~0xF0;
    mdmcfg4 |= (CHANBW_M << 4);
    mdmcfg4 |= (CHANBW_E << 6);
    writeAccess(CC110L_MDMCFG4,&mdmcfg4,1);

    // TODO error handling
    return COMETOS_SUCCESS;
}

cometos_error_t CC110lApi::setModulation(Modulation modulation)
{
    uint8_t mdmcfg2;
    readAccess(CC110L_MDMCFG2,&mdmcfg2,1); // other values should stay equal
    mdmcfg2 &= ~0x70;

    if(modulation == Modulation::FSK2) {
        mdmcfg2 |= 0 << 4;
    }
    else if(modulation == Modulation::GFSK) {
        mdmcfg2 |= 1 << 4;
    }
    else if(modulation == Modulation::OOK) {
        mdmcfg2 |= 3 << 4;
    }
    else {
        return COMETOS_ERROR_INVALID;
    }

    writeAccess(CC110L_MDMCFG2,&mdmcfg2,1);
    return COMETOS_SUCCESS;
}

cometos_error_t CC110lApi::setCCAMode(CCAMode ccaMode)
{
    uint8_t mcsm1;
    readAccess(CC110L_MCSM1,&mcsm1,1); // other values should stay equal
    mcsm1 &= ~0x30;

    if(ccaMode == CCAMode::ALWAYS) {
        mcsm1 |= 0 << 4;
    }
    else if(ccaMode == CCAMode::RSSI) {
        mcsm1 |= 1 << 4;
    }
    else if(ccaMode == CCAMode::RECEIVING) {
        mcsm1 |= 2 << 4;
    }
    else if(ccaMode == CCAMode::RSSI_RECEIVING) {
        mcsm1 |= 3 << 4;
    }
    else {
        return COMETOS_ERROR_INVALID;
    }

    writeAccess(CC110L_MCSM1,&mcsm1,1);
    return COMETOS_SUCCESS;
}

cometos_error_t CC110lApi::setCoding(Coding coding)
{
    uint8_t mdmcfg2;
    readAccess(CC110L_MDMCFG2,&mdmcfg2,1); // other values should stay equal
    mdmcfg2 &= ~(1 << 3);

    if(coding == Coding::UNCODED) {
        mdmcfg2 |= 0 << 3;
    }
    else if(coding == Coding::MANCHESTER) {
        mdmcfg2 |= 1 << 3;
    }
    else {
        return COMETOS_ERROR_INVALID;
    }

    writeAccess(CC110L_MDMCFG2,&mdmcfg2,1);
    return COMETOS_SUCCESS;
}

cometos_error_t CC110lApi::setSync(Sync sync)
{
    uint8_t mdmcfg2;
    readAccess(CC110L_MDMCFG2,&mdmcfg2,1); // other values should stay equal
    mdmcfg2 &= ~0x7;
    mdmcfg2 |= (uint8_t)sync;

    writeAccess(CC110L_MDMCFG2,&mdmcfg2,1);
    return COMETOS_SUCCESS;
}

cometos_error_t CC110lApi::setPreamble(uint8_t bytes)
{
    uint8_t mdmcfg1;
    readAccess(CC110L_MDMCFG1,&mdmcfg1,1); // other values should stay equal
    mdmcfg1 &= ~0x70;

    switch(bytes) {
    case 2:
        mdmcfg1 |= (0 << 4);
    break;
    case 3:
        mdmcfg1 |= (1 << 4);
    break;
    case 4:
        mdmcfg1 |= (2 << 4);
    break;
    case 6:
        mdmcfg1 |= (3 << 4);
    break;
    case 8:
        mdmcfg1 |= (4 << 4);
    break;
    case 12:
        mdmcfg1 |= (5 << 4);
    break;
    case 16:
        mdmcfg1 |= (6 << 4);
    break;
    case 24:
        mdmcfg1 |= (7 << 4);
    break;
    default:
    return COMETOS_ERROR_INVALID;
    }

    writeAccess(CC110L_MDMCFG1,&mdmcfg1,1);
    return COMETOS_SUCCESS;
}

cometos_error_t CC110lApi::setCRC(bool enable)
{
    uint8_t data;
    readAccess(CC110L_PKTCTRL0,&data,1);
    if(enable) {
        data |= (1 << 2);
    }
    else {
        data &= ~(1 << 2);
    }
    writeAccess(CC110L_PKTCTRL0,&data,1);
    return COMETOS_SUCCESS;
}

cometos_error_t CC110lApi::setFixedLength(uint8_t length)
{
    uint8_t data;
    readAccess(CC110L_PKTCTRL0,&data,1);
    data &= ~0x3;
    writeAccess(CC110L_PKTCTRL0,&data,1);
    writeAccess(CC110L_PKTLEN,&length,1);
    hasFixedLength = true;
    fixedLength = length;
    return COMETOS_SUCCESS;
}

cometos_error_t CC110lApi::setFrequencyOffset(int8_t offset)
{
#ifdef BOARD_lorasender
    offset += 7;
#endif
#ifdef BOARD_AutoRNode
    offset += 19;
#endif

    writeAccess(CC110L_FSCTRL0,(uint8_t*)&offset,1);
    return COMETOS_SUCCESS;
}

cometos_error_t CC110lApi::setRXAttenuation(uint8_t attenuation)
{
    attenuation = (attenuation+3)/6;
    if(attenuation > 3) {
        return COMETOS_ERROR_INVALID;
    }

    uint8_t data;
    readAccess(CC110L_FIFOTHR,(uint8_t*)&data,1);
    data &= ~(3 << 4);
    data |= (attenuation << 4);
    writeAccess(CC110L_FIFOTHR,(uint8_t*)&data,1);
    return COMETOS_SUCCESS;
}

}
