/*
* Copyright (c) 2007, Vanderbilt University
* Copyright (c) 2010, University of Szeged
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*
* - Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
* - Redistributions in binary form must reproduce the above
* copyright notice, this list of conditions and the following
* disclaimer in the documentation and/or other materials provided
* with the distribution.
* - Neither the name of University of Szeged nor the names of its
* contributors may be used to endorse or promote products derived
* from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
* COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*
* Author: Miklos Maroti
* Author:Andras Biro
* Author: Andreas Weigel (modifications for CometOS)
*/

#include "RFA1DriverLayer.h"

#include <stdint.h>
#include <string.h>

#include "palLocalTime.h"
#include "cometos.h"
#include "RadioConfig.h"
#include "tasklet.h"
#include "logging.h"
#include "palExec.h"
#include "atm128hardware.h"
#include "palLed.h"
#include "mac_constants.h"
#include "tosUtil.h"
#include "palPers.h"
#include "OutputStream.h"
#include "timer3.h"
#include "pinEventOutput.h"
#include <util/delay.h>
#include <util/atomic.h>


#define RADIO_ASSERT(x) ASSERT(x)
//#define RADIO_ASSERT(x)

static const uint8_t MIN_FRAME_LEN = 5;   // size of an ACK frame
static const uint8_t MAX_FRAME_LEN = 127; // maximum PHY payload of 802.14.5
static const uint8_t FCS_FIELD_SIZE = 2;  // size of frame check sequence

static volatile bool radioStateChangeInProgress = false;

/**
 * Port of the TinyOS radio driver, including SoftwareAck, RandomBackoff and
 * Csma layers to fit the mac_interface of CometOS. The flexibility and
 * modularity of TinyOS' radio stack is quite lost here. Additionally,
 * we have to consider that radio state and channel changes here are
 * split-phase
 *
 * NOTE: this radio driver is not as flexible as it used to be within TinyOS.
 * NOTE: The radio driver correctly separates phy and mac and is only responsible
 *       for the actual PHY functions. This means that the actual interpretation
 *       of the data has to be done by upper layers, which differs from the
 * TODO: radioState changes won't work, because PLL_ON interrupt is not
 *       enabled because it caused problems with putting the driver into
 *       active RX mode
 */


//void radioSend_sendDone(const message_t* msg, mac_result_t result, const mac_txInfo_t* info) {
//    mac_cbSendDone(msg->data, result, info);
//}

void __attribute__((weak)) radioCCA_done(cometos_error_t result) {

}

cometos_error_t radioState_turnOn();

/**
 * default implementation of radioState_done
 * when using radioState changes exclusively within initialization, this
 * should work well enough
 */
void radioState_done() {
    radioStateChangeInProgress = false;
}


/**
 * default implementation of radioSend_ready()
 */
void __attribute__((weak)) radioSend_ready() {

}


/*----------------- STATE -----------------*/

//  tasklet_norace

enum
{
    STATE_P_ON = 0,
    STATE_SLEEP = 1,
    STATE_SLEEP_2_TRX_OFF = 2,
    STATE_TRX_OFF = 3,
    STATE_TRX_OFF_2_RX_ON = 4,
    STATE_RX_ON = 5,
    STATE_BUSY_TX_2_RX_ON = 6,
};
static uint8_t radio_state = STATE_TRX_OFF;

//  tasklet_norace
uint8_t cmd;
enum
{
    CMD_NONE = 0,    // the state machine has stopped
    CMD_TURNOFF = 1,    // goto SLEEP state
    CMD_STANDBY = 2,    // goto TRX_OFF state
    CMD_TURNON = 3,    // goto RX_ON state
    CMD_TRANSMIT = 4,    // currently transmitting a message
    CMD_RECEIVE = 5,    // currently receiving a message
    CMD_CCA = 6,    // performing clear channel assessment
    CMD_CHANNEL = 7,    // changing the channel
    CMD_SIGNAL_DONE = 8,  // signal the end of the state transition
    CMD_DOWNLOAD = 9,    // download the received message
};

enum {
    IRQ_NONE=0,
    IRQ_AWAKE=1,
    IRQ_TX_END=2,
    IRQ_XAH_AMI=4,
    IRQ_CCA_ED_DONE=8,
    IRQ_RX_END=16,
    IRQ_RX_START=32,
    IRQ_PLL_UNLOCK=64,
    IRQ_PLL_LOCK=128,
};



//enum {
//    // this disables the RFA1RadioOffP component
//    RFA1RADIOON = unique("RFA1RadioOn"),
//};

//  norace
static volatile uint8_t radioIrq = IRQ_NONE;

//static mac_phyPacketInfo_t rxInfo;

//static mac_txInfo_t txInfo;

//  tasklet_norace
static uint8_t txPower;
//  tasklet_norace
static uint8_t tos_channel;

/** workaround to have a single large buffer available -- problem is that
 * we can get a buffer from cometos MAL only for the actual mac payload,
 * but we need one for whole data packets. as this driver is cleanly
 * layered, the RadioDriver does not distinguish between mac header and
 * mac payload and we have to be able to receive acks as well as data packets
 * and pass to the next higher layer (SoftwareAckLayer)
 *
 * TODO if we can help this somehow, we should, its wasting a whole frame
 *      buffer of memory
 */
static uint8_t rxBuf[MAC_MAX_PAYLOAD_SIZE + MAC_HEADER_SIZE];
static message_t rxMsgImpl;

static message_t* rxMsg = &rxMsgImpl;

static message_t* txMsg = NULL;

//  tasklet_norace
static uint8_t rssiClear;
//  tasklet_norace
static uint8_t rssiBusy;


/*----------------- DUMMIES --------------*/
mac_result_t mac_setMode(mac_txMode_t mode) {return MAC_SUCCESS;}
mac_txMode_t mac_getMode() {return MAC_MODE_CCA | MAC_MODE_AUTO_ACK | MAC_MODE_BACKOFF;}


/*----------------- INIT -----------------*/
#include "OutputStream.h"
mac_result_t RFA1Driver_init(mac_nodeId_t myAddr, mac_networkId_t nwkId,
        mac_channel_t channel, mac_txMode_t mode, mac_ackCfg_t *ackCfg,
        mac_backoffCfg_t *backoffCfg)
{
    rxMsg->data = rxBuf;
    mac_result_t result = MAC_SUCCESS;
    // these are just good approximates
    rssiClear = 0;
    rssiBusy = 90;

    // outputting state via high nibble of PORTB
    EVENT_OUTPUT_INIT();

    TRXPR |= 1 << TRXRST;
    while (TRANSCEIVER_STATE != TRX_OFF) {
        ;
    }

    radio_state = STATE_TRX_OFF;

    CCA_THRES=RFA1_CCA_THRES_VALUE;

#ifdef RFA1_ENABLE_PA
    SET_BIT(DDRG,0);    // DIG3
    CLR_BIT(PORTG,0);
    SET_BIT(DDRF, 3);   // DIG0
    CLR_BIT(PORTF, 3);
#endif

#ifdef RFA1_ENABLE_EXT_ANT_SW
    SET_BIT(DDRG, 1);// DIG1
    CLR_BIT(PORTG,1);
    SET_BIT(DDRF, 2); // DIG2
    CLR_BIT(PORTF, 2);
#endif
#ifdef RFA1_DATA_RATE
#if RFA1_DATA_RATE == 250
    TRX_CTRL_2 = (TRX_CTRL_2 & 0xfc) | 0;
#elif RFA1_DATA_RATE == 500
    TRX_CTRL_2 = (TRX_CTRL_2 & 0xfc) | 1;
#elif RFA1_DATA_RATE == 1000
    TRX_CTRL_2 = (TRX_CTRL_2 & 0xfc) | 2;
#elif RFA1_DATA_RATE == 2000
    TRX_CTRL_2 = (TRX_CTRL_2 & 0xfc) | 3;
#else
  #error Unsupported RFA1_DATA_RATE (supported: 250, 500, 1000, 2000. default is 250)
#endif
#endif
    PHY_TX_PWR = RFA1_PA_BUF_LT | RFA1_PA_LT | (RFA1_DEF_RFPOWER&TX_PWR_MASK)<<TX_PWR0;

    txPower = RFA1_DEF_RFPOWER & TX_PWR_MASK;
    tos_channel = channel;
    TRX_CTRL_1 |= 1<<TX_AUTO_CRC_ON;

    // channel is set later explicitly
    PHY_CC_CCA = RFA1_CCA_MODE_VALUE;

    //    SfdCapture_setMode(ATMRFA1_CAPSC_ON);

    // instead of going to sleep, we go directly to RX mode for now
//    SET_BIT(TRXPR,SLPTR);
//    radio_state = STATE_SLEEP;

    result = mac_setNodeId(myAddr);
    if (result != MAC_SUCCESS) {
        return result;
    }

    result = mac_setNetworkId(nwkId);
    if (result != MAC_SUCCESS) {
        return result;
    }

    result = mac_setChannel(channel);
    if (result != MAC_SUCCESS) {
        return result;
    }

    result = mac_setMode(mode);
    if (result != MAC_SUCCESS) {
        return result;
    }

    // set power to maximum value
    mac_setTxPower(mac_getMaxTxPowerLvl());

    // deactivate RX_OVERRIDE feature
    RX_SYN = 0;

//    radioState_turnOn();
    // enter TRX state from TRX_OFF state
    TRX_STATE = CMD_RX_ON;

    while (TRANSCEIVER_STATE != RX_ON)
        ;

    radio_state = STATE_RX_ON;
    // clear pending interrupts and then enable initial interrupts
    IRQ_STATUS = 0xFF;
//    IRQ_MASK = 1<<PLL_LOCK_EN | 1<<TX_END_EN | 1<<RX_END_EN | 1<< RX_START_EN | 1<<CCA_ED_DONE_EN;
    IRQ_MASK = 1<<TX_END_EN | 1<<RX_END_EN | 1<< RX_START_EN | 1<<CCA_ED_DONE_EN;
    return MAC_SUCCESS;
}

/*----------------- CHANNEL -----------------*/

//tasklet_async command
 uint8_t radioState_getChannel()
{
return tos_channel;
}

//tasklet_async command
cometos_error_t radioState_setChannel(uint8_t c) {
    c &= CHANNEL_MASK;

    if( cmd != CMD_NONE )
        return MAC_ERROR_BUSY;
    else if( tos_channel == c )
        return MAC_ERROR_ALREADY;

    tos_channel = c;
    cmd = CMD_CHANNEL;
    tasklet_schedule();

    return MAC_SUCCESS;
}

inline void changeChannel() {
    RADIO_ASSERT( cmd == CMD_CHANNEL );
    RADIO_ASSERT( radio_state == STATE_SLEEP || radio_state == STATE_TRX_OFF || radio_state == STATE_RX_ON );

    PHY_CC_CCA=RFA1_CCA_MODE_VALUE|tos_channel;

    if( radio_state == STATE_RX_ON )
        radio_state = STATE_TRX_OFF_2_RX_ON;
    else
        cmd = CMD_SIGNAL_DONE;
}

/*----------------- TURN ON/OFF -----------------*/

inline void changeState() {
    if( (cmd == CMD_STANDBY || cmd == CMD_TURNON) && radio_state == STATE_SLEEP ) {
        RADIO_ASSERT( ! radioIrq );

        IRQ_STATUS = 0xFF;
        IRQ_MASK = 1<<AWAKE_EN;
        CLR_BIT(TRXPR,SLPTR);
//        McuPowerState_update();

        radio_state = STATE_SLEEP_2_TRX_OFF;
    } else if( cmd == CMD_TURNON && radio_state == STATE_TRX_OFF ) {
        RADIO_ASSERT( ! radioIrq );

        IRQ_MASK = 1<<PLL_LOCK_EN | 1<<TX_END_EN | 1<<RX_END_EN | 1<< RX_START_EN | 1<<CCA_ED_DONE_EN;
//        McuPowerState_update();

        TRX_STATE = CMD_RX_ON;
        #ifdef RFA1_ENABLE_PA
        SET_BIT(TRX_CTRL_1, PA_EXT_EN);
        #endif

        #ifdef RFA1_ENABLE_EXT_ANT_SW
        #ifdef RFA1_ANT_DIV_EN
        ANT_DIV = 0x7f & (1<<ANT_DIV_EN | 1<<ANT_EXT_SW_EN);
        #elif defined(RFA1_ANT_SEL1)
        ANT_DIV = 0x7f & (1<<ANT_EXT_SW_EN | 1<<ANT_CTRL0);
        #elif defined(RFA1_ANT_SEL0)
        ANT_DIV = 0x7f & (1<<ANT_EXT_SW_EN | 2<<ANT_CTRL0);
        #else
        #error Neighter antenna is selected with ANT_EXT_SW_EN. You can choose between RFA1_ANT_DIV_EN, RFA1_ANT_SEL0, RFA1_ANT_SEL1
        #endif
        #endif
        radio_state = STATE_TRX_OFF_2_RX_ON;
//        ExtAmpControl_start();
    } else if( (cmd == CMD_TURNOFF || cmd == CMD_STANDBY) && radio_state == STATE_RX_ON ) {
        #ifdef RFA1_ENABLE_PA
        CLR_BIT(TRX_CTRL_1, PA_EXT_EN);
        #endif

        #ifdef RFA1_ENABLE_EXT_ANT_SW
        ANT_DIV=3; //default value
        #endif
        TRX_STATE = CMD_FORCE_TRX_OFF;

        IRQ_MASK = 0;
        radioIrq = IRQ_NONE;

//        McuPowerState_update();

        radio_state = STATE_TRX_OFF;
//        ExtAmpControl_stop();
    }

    if( cmd == CMD_TURNOFF && radio_state == STATE_TRX_OFF ) {
        SET_BIT(TRXPR,SLPTR);
        radio_state = STATE_SLEEP;
        cmd = CMD_SIGNAL_DONE;
    } else if( cmd == CMD_STANDBY && radio_state == STATE_TRX_OFF ) {
        IRQ_MASK = 0;
//        McuPowerState_update();

        cmd = CMD_SIGNAL_DONE;
    }
}

//tasklet_async command
cometos_error_t radioState_turnOff() {
    if( cmd != CMD_NONE ) {
        return MAC_ERROR_BUSY;
    } else if( radio_state == STATE_SLEEP ) {
        return MAC_ERROR_ALREADY;
    }
    cmd = CMD_TURNOFF;
    tasklet_schedule();
    return MAC_SUCCESS;

    /*----------------- TRANSMIT -----------------*/

    enum {
    // 16 us delay (1 tick), 4 bytes preamble (2 ticks each), 1 byte SFD (2 ticks)
    TX_SFD_DELAY = 11,
    };

}

//tasklet_async command
cometos_error_t radioState_standby() {
    if( cmd != CMD_NONE  )
        return MAC_ERROR_BUSY;
    else if( radio_state == STATE_TRX_OFF )
        return MAC_ERROR_ALREADY;

    cmd = CMD_STANDBY;
    tasklet_schedule();

    return MAC_SUCCESS;
}

//tasklet_async command
cometos_error_t radioState_turnOn() {
    if( cmd != CMD_NONE )
        return MAC_ERROR_BUSY;
    else if( radio_state == STATE_RX_ON )
        return MAC_ERROR_ALREADY;
    cmd = CMD_TURNON;
    tasklet_schedule();

    return MAC_SUCCESS;
}

//default tasklet_async event void radioState_done() { }

/*----------------- TRANSMIT -----------------*/

enum {
// 16 us delay (1 tick), 4 bytes preamble (2 ticks each), 1 byte SFD (2 ticks)
TX_SFD_DELAY = 11,
};

//tasklet_async command
mac_result_t radioSend_send(message_t* msg)
{
    if( cmd != CMD_NONE || radio_state != STATE_RX_ON || radioIrq ) {
        return MAC_ERROR_BUSY;
    }

    TRX_STATE = CMD_PLL_ON;


	uint8_t state = TRANSCEIVER_STATE;
    //_delay_us(1.5);
    // wait for PLL_ON or RX_BUSY, which should be the only possible states
    // here, assuming that we are in RX_ON before
    while (state != PLL_ON && state != BUSY_RX)
		state = TRANSCEIVER_STATE;

    // RX_BUSY --> we have missed an incoming message in this short amount of time
    if( state != PLL_ON ) {
		// we should then really be in BUSY_RX state
        if (state != BUSY_RX) {
            //cometos::getCout() << "s=" << (int) state << cometos::endl;

        }
        RADIO_ASSERT( state == BUSY_RX);

        TRX_STATE = CMD_RX_ON;
        return MAC_ERROR_BUSY;
    }

    EVENT_OUTPUT_WRITE(PEO_RADIO_TX_START);

    //we change this part here to force pll on. on very rare occasions we
    //might thereby lose a frame arriving between the check above and
    //the command itself, but this should be neglectable and get rid of the
    //problems concerned with the RADIO_ASSERT
    //TRX_STATE = CMD_FORCE_PLL_ON;
    //while (TRANSCEIVER_STATE != PLL_ON)
        //;
//
    //if(radioIrq) {
        //rxKillsByTx++;
        //cometos::getCout() << "rx_kill: " << (int) rxKillsByTx << cometos::endl;
    //}
    //clear radioIrq to prevent service_radio to be called again
    //by the tasklet loop in this rare case
    //where an RX_START irq occurred between check and CMD_FORCE_PLL_ON
    //radioIrq = 0;
    //now we can be sure that we are in a correct state for sending

    txMsg = msg;
    txMsg->txInfo.tsData.isValid = false;
    // write the PHR length field
    TRXFBST = msg->phyFrameLen;

    palExec_atomicBegin();
    {
        TRX_STATE = CMD_TX_START;
    }
    palExec_atomicEnd();

//    time += TX_SFD_DELAY;

    RADIO_ASSERT( ! radioIrq );

    // copy header buf to tx buffer
//    memcpy((void*)(&TRXFBST+1), headerBuf, MAC_HEADER_SIZE-2);

    // then upload the whole payload
    memcpy((void*)(&TRXFBST+1), msg->data, msg->phyPayloadLen);

    // go back to RX_ON radio_state when finished
    TRX_STATE=CMD_RX_ON;


//    cometos::getCout() << "tx:" <<(int) msg->phyFrameLen << "|" << tosUtil_isAckFrame(msg) << cometos::endl;
    // wait for the TRX_END interrupt
    radio_state = STATE_BUSY_TX_2_RX_ON;
    cmd = CMD_TRANSMIT;

    return MAC_SUCCESS;
}

//default tasklet_async event void radioSend_sendDone(cometos_error_t error) { }
//default tasklet_async event void radioSend_ready() { }

/*----------------- CCA -----------------*/

//tasklet_async command
cometos_error_t radioCCA_request() {
    if( cmd != CMD_NONE || radio_state != STATE_RX_ON )
        return MAC_ERROR_BUSY;

    // see Errata 38.5.5 datasheet
    TRX_STATE=CMD_PLL_ON;
    //TODO: test&optimize this
    _delay_us(1);
    if( (TRX_STATUS & TRX_STATUS_MASK) != PLL_ON )
        return MAC_ERROR_BUSY;
    SET_BIT(RX_SYN,RX_PDT_DIS);
    TRX_STATE=CMD_RX_ON;
    //end of workaround

    cmd = CMD_CCA;
    PHY_CC_CCA = 1 << CCA_REQUEST | RFA1_CCA_MODE_VALUE | tos_channel;

    return MAC_SUCCESS;
}

//default tasklet_async event void radioCCA_done(cometos_error_t error) { }

/*----------------- RECEIVE -----------------*/

//TODO: RX_SAFE_MODE with define
inline void downloadMessage()
{
    bool sendSignal = false;

    rxMsg->phyFrameLen = TST_RX_LENGTH;
    rxMsg->phyPayloadLen = rxMsg->phyFrameLen - 2; //< size of CRC footer
//    uint8_t* data = NULL;

    volatile uint8_t* radioFrameBuf = &TRXFBST;

    // rxBuffer always has to be available, we drop at higher layers
    RADIO_ASSERT(rxMsg->data != NULL);
//    palExec_atomicBegin();
//    bool bufAvailable = rxMsg->data != NULL;
//    palExec_atomicEnd();

//    uint16_t dst = radioFrameBuf[5] | (radioFrameBuf[6] << 8);
//    uint16_t src = radioFrameBuf[7] | (radioFrameBuf[8] << 8);
//    cometos::getCout() << "rcvd:" << (int) rxMsg->phyFrameLen << cometos::endl;
    if( (PHY_RSSI & (1<<RX_CRC_VALID))
            && (rxMsg->phyPayloadLen >= 3 && rxMsg->phyFrameLen - MAC_HEADER_SIZE <=  MAC_PACKET_BUFFER_SIZE)) {

//        data = getPayload(rxMsg);
//        getHeader(rxMsg)->length = length;

        // memory is fast, no point optimizing header check
        memcpy(rxMsg->data, (uint8_t*) radioFrameBuf, rxMsg->phyPayloadLen);
//        cometos::getCout() << "isAck:" << tosUtil_isAckFrame(rxMsg) << cometos::endl;
//        palExec_atomicBegin();
//        data = rxBuf;
//        rxBuf = NULL;
//        palExec_atomicEnd();

        rxMsg->rxInfo.lqi = (uint8_t)*(&TRXFBST+TST_RX_LENGTH);
        rxMsg->rxInfo.lqiIsValid = true;
        sendSignal = true;
    } else {
        EVENT_OUTPUT_WRITE(PEO_RADIO_RX_DROPPED);
    }

    TRX_STATE = CMD_RX_ON;
    radio_state = STATE_RX_ON;

    cmd = CMD_NONE;

    // signal only if it has passed the CRC check
    if( sendSignal ) {
        rxMsg = radioReceive_receive(rxMsg);
    }

}

//default tasklet_async event bool RadioReceive_header(message_t* msg)
//{
//return TRUE;
//}
//
//default tasklet_async event message_t* RadioReceive_receive(message_t* msg)
//{
//return msg;
//}

/*----------------- IRQ -----------------*/
void serviceRadio() {
//    uint32_t time;
    uint8_t irq;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        // time =  SfdCapture_get();
        irq = radioIrq;
        radioIrq = IRQ_NONE;
    }

//    #ifdef RFA1_RSSI_ENERGY
    // check this early before the PHY_ED_LEVEL register is overwritten
    if( irq == IRQ_RX_END ) {
        rxMsg->rxInfo.rssi = PHY_ED_TO_RSSI_DBM(PHY_ED_LEVEL);
//        PacketRSSI_set(rxMsg, PHY_ED_LEVEL);
    } else {
        rxMsg->rxInfo.rssi = RSSI_INVALID;
//        PacketRSSI_clear(rxMsg);
//    #endif
    }

    if( (irq & IRQ_PLL_LOCK) != 0 ) {
        if( cmd == CMD_TURNON || cmd == CMD_CHANNEL ) {
            RADIO_ASSERT( radio_state == STATE_TRX_OFF_2_RX_ON );

            radio_state = STATE_RX_ON;
            cmd = CMD_SIGNAL_DONE;
        }
        else if( cmd == CMD_TRANSMIT ) {
            RADIO_ASSERT( radio_state == STATE_BUSY_TX_2_RX_ON );
        }
        else {
//            cometos::getCout() << radio_state << "|" << cmd << "|" << irq << cometos::endl;
            RADIO_ASSERT(false);
        }
    }

    if( cmd == CMD_TRANSMIT && (irq & IRQ_TX_END) != 0 ) {
        EVENT_OUTPUT_WRITE(PEO_RADIO_TX_END);
        RADIO_ASSERT( radio_state == STATE_BUSY_TX_2_RX_ON );

        txMsg->txInfo.tsData.isValid = true;
        radio_state = STATE_RX_ON;
        cmd = CMD_NONE;
        radioSend_sendDone(txMsg, MAC_SUCCESS);

        // TODO: we could have missed a received message
        RADIO_ASSERT( ! (irq & IRQ_RX_START) );
    }

    if( (irq & IRQ_RX_START) != 0 ) {

        if( cmd == CMD_CCA ) {
            radioCCA_done(MAC_ERROR_FAIL);
            cmd = CMD_NONE;
        }

        if( cmd == CMD_NONE ) {
            EVENT_OUTPUT_WRITE(PEO_RADIO_RX_START);
            RADIO_ASSERT( radio_state == STATE_RX_ON );

            // the most likely place for busy channel and good SFD, with no other interrupts
            if( irq == IRQ_RX_START ) {
//                temp = PHY_RSSI & RSSI_MASK;
//                rssiBusy += temp - (rssiBusy >> 2);
//
//
//                PacketTimeStamp_set(rxMsg, time);
//                rxMsg->rxInfo.tsData.isValid = true;
//                rxMsg->rxInfo.tsData.ts = palLocalTime_get();

//            #ifndef RFA1_RSSI_ENERGY
//                PacketRSSI_set(rxMsg, temp);
//            #endif
            } else {
                rxMsg->rxInfo.tsData.isValid = false;

                // some other interrupt active? invalidating rssi/timestamp?
//                PacketTimeStamp_clear(rxMsg);
//
//             #ifndef RFA1_RSSI_ENERGY
//                PacketRSSI_clear(rxMsg);
//             #endif
            }

            cmd = CMD_RECEIVE;
        } else {
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                if (cmd != CMD_TURNOFF) {
                    //cometos::getCout() << (int) cmd << cometos::endl;
                }
            }
            RADIO_ASSERT( cmd == CMD_TURNOFF);
        }
    }

    if( (irq & IRQ_RX_END) != 0 ) {
        if (cmd == CMD_RECEIVE ) {
            EVENT_OUTPUT_WRITE(PEO_RADIO_RX_END);
//            while (TRANSCEIVER_STATE != PLL_ON) {
//                ;
//            }
            RADIO_ASSERT( radio_state == STATE_RX_ON );

            rxMsg->rxInfo.tsData.isValid = true;
            // the most likely place for clear channel (hope to avoid acks)
            rssiClear += (PHY_RSSI & RSSI_MASK) - (rssiClear >> 2);

            cmd = CMD_DOWNLOAD;
        } else {
            // catch a situation where we are not in expectation of a reception
            // but still have received an RX_END interrupt -- reactivate
            // reception of frames
            TRX_STATE = CMD_RX_ON;
        }
    }

    if( (irq & IRQ_AWAKE) != 0 ) {
        if( radio_state == STATE_SLEEP_2_TRX_OFF && (cmd==CMD_STANDBY || cmd==CMD_TURNON) ) {
            radio_state = STATE_TRX_OFF;
        }
        else {
            RADIO_ASSERT(false);
        }
    }

    if( (irq & IRQ_CCA_ED_DONE) != 0 ) {
        if( cmd == CMD_CCA ) {
            // workaround, see Errata 38.5.5 datasheet
            CLR_BIT(RX_SYN,RX_PDT_DIS);

            cmd = CMD_NONE;

            RADIO_ASSERT( radio_state == STATE_RX_ON );
            RADIO_ASSERT( (TRX_STATUS & TRX_STATUS_MASK) == RX_ON );

            radioCCA_done( (TRX_STATUS & CCA_DONE) ? ((TRX_STATUS & CCA_STATUS) ? MAC_SUCCESS : MAC_ERROR_BUSY) : MAC_ERROR_FAIL );
        } else {
            RADIO_ASSERT(false);
        }
    }
}


// we changed ISRs to be non-interuptable until they call the tasklet_schedule
// function. Rationale: In very rare occasions, we get RX_START which is
// immediately interrupted (before setting the radioIrq variable) by another
// interrupt. RX_END then also arrives before RX_START is further executed.
// This leads to an RADIO_ASSERT as soon as the next RX_START is triggered,
// because the cmd will be CMD_RECEIVE. Additionally, it causes this module
// to starve (no TX possible during cmd = CMD_RECEIVE) until the next RX_START
// is triggered (which in worst case may depend on this node doing some TX)
// Therefore, we make the ISRs again blocking, and only reactivate global
// interrupts before we call tasklet_schedule


/**
* Indicates the completion of a frame transmission
*/
//AVR_NONATOMIC_HANDLER(TRX24_TX_END_vect){
//ISR(TRX24_TX_END_vect, ISR_NOBLOCK){
ISR(TRX24_TX_END_vect){
    // for now, we use only millisec precision time sync
    txMsg->txInfo.tsData.ts = palLocalTime_get();
    RADIO_ASSERT( ! radioIrq );
//    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        radioIrq |= IRQ_TX_END;
//    }
    sei();
    tasklet_schedule();
}

//static uint8_t* stack;
//static uint32_t pcVal;
/**
* Indicates the completion of a frame reception
*/
//AVR_NONATOMIC_HANDLER(TRX24_RX_END_vect){
//ISR(TRX24_RX_END_vect, ISR_NOBLOCK){
ISR(TRX24_RX_END_vect){
    // get ts of rx end -- this is not 100% exact here and we should
    // probably use the hardware features to get an exact timestamp
    rxMsg->rxInfo.tsData.ts = palLocalTime_get();
    //uint8_t * stack= (reinterpret_cast<uint8_t*>(SP))+31;
//    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
//        TRX_STATE = CMD_PLL_ON;
//    }
//    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if (radioIrq) {
            uint8_t len = TST_RX_LENGTH;
            if (len < MIN_FRAME_LEN || len > MAX_FRAME_LEN) {
                radioIrq = IRQ_NONE;
                return;
            } else {
                //uint32_t pcVal = (((uint32_t) (stack[0] & 0x1)) << 17) | (((uint32_t) stack[1]) << 9) | (((uint32_t) stack[2]) << 1);
                //cometos::getCout() << cometos::dec << (uint16_t) RX_SYN << "|" << (uint16_t) radioIrq << "|" << (uint16_t) len << "|" << timer3_get()<< "|" << cometos::hex << pcVal << cometos::endl;
                RADIO_ASSERT( ! radioIrq );
            }
        }
        radioIrq |= IRQ_RX_END;
//    }
    sei();
    tasklet_schedule();
}



/**
* Indicates the start of a PSDU reception. The TRX_STATE changes
* to BUSY_RX, the PHR is ready to be read from Frame Buffer
*/
//AVR_NONATOMIC_HANDLER(TRX24_RX_START_vect){
//ISR(TRX24_RX_START_vect, ISR_NOBLOCK){
ISR(TRX24_RX_START_vect){
//    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
//        if (radioIrq) {
//            cometos::getCout() << (int) radioIrq << cometos::endl;
//        }
        RADIO_ASSERT( ! radioIrq );
        timer3_start();
        radioIrq |= IRQ_RX_START;
//    }
    sei();
    tasklet_schedule();
}


/**
* Indicates PLL lock
*/
//AVR_NONATOMIC_HANDLER(TRX24_PLL_LOCK_vect){
//ISR(TRX24_PLL_LOCK_vect, ISR_NOBLOCK){
ISR(TRX24_PLL_LOCK_vect){
    RADIO_ASSERT( ! radioIrq );
//    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        radioIrq |= IRQ_PLL_LOCK;
//    }
    sei();
    tasklet_schedule();
}

/**
* indicates sleep/reset->trx_off mode change
*/
//AVR_NONATOMIC_HANDLER(TRX24_AWAKE_vect){
//ISR(TRX24_AWAKE_vect, ISR_NOBLOCK){
ISR(TRX24_AWAKE_vect) {
    RADIO_ASSERT( ! radioIrq );
//    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        radioIrq |= IRQ_AWAKE;
//    }
    sei();
    tasklet_schedule();
}

/**
* indicates CCA ED done
*/
//AVR_NONATOMIC_HANDLER(TRX24_CCA_ED_DONE_vect){
//ISR(TRX24_CCA_ED_DONE_vect, ISR_NOBLOCK){
ISR(TRX24_CCA_ED_DONE_vect){
    RADIO_ASSERT( ! radioIrq );
//    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        radioIrq |= IRQ_CCA_ED_DONE;
//    }
    sei();
    tasklet_schedule();
}

///**
//* Indicates the completion of a frame transmission
//*/
////AVR_NONATOMIC_HANDLER(TRX24_TX_END_vect){
//ISR(TRX24_TX_END_vect, ISR_NOBLOCK){
//    // for now, we use only millisec precision time sync
//    txMsg->txInfo.tsData.ts = palLocalTime_get();
//    RADIO_ASSERT( ! radioIrq );
//    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
//        radioIrq |= IRQ_TX_END;
//    }
//    tasklet_schedule();
//}
//
//uint8_t* stack;
//uint32_t pcVal;
///**
//* Indicates the completion of a frame reception
//*/
////AVR_NONATOMIC_HANDLER(TRX24_RX_END_vect){
//ISR(TRX24_RX_END_vect, ISR_NOBLOCK){
//    // get ts of rx end -- this is not 100% exact here and we should
//    // probably use the hardware features to get an exact timestamp
//    rxMsg->rxInfo.tsData.ts = palLocalTime_get();
//    stack= (reinterpret_cast<uint8_t*>(SP))+31;
////    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
////        TRX_STATE = CMD_PLL_ON;
////    }
//    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
//        if (radioIrq) {
//			uint8_t len = TST_RX_LENGTH;
//			if (len < MIN_FRAME_LEN || len > MAX_FRAME_LEN) {
//				radioIrq = IRQ_NONE;
//				return;
//			} else {
//				pcVal = (((uint32_t) (stack[0] & 0x1)) << 17) | (((uint32_t) stack[1]) << 9) | (((uint32_t) stack[2]) << 1);
//				cometos::getCout() << cometos::dec << (uint16_t) RX_SYN << "|" << (uint16_t) radioIrq << "|" << (uint16_t) len << "|" << timer3_get()<< "|" << cometos::hex << pcVal << cometos::endl;
//				RADIO_ASSERT( ! radioIrq );
//			}
//        }
//        radioIrq |= IRQ_RX_END;
//    }
//    tasklet_schedule();
//}
//
//
//
///**
//* Indicates the start of a PSDU reception. The TRX_STATE changes
//* to BUSY_RX, the PHR is ready to be read from Frame Buffer
//*/
////AVR_NONATOMIC_HANDLER(TRX24_RX_START_vect){
//ISR(TRX24_RX_START_vect, ISR_NOBLOCK){
//    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
//        if (radioIrq) {
//            cometos::getCout() << (int) radioIrq << cometos::endl;
//        }
//        RADIO_ASSERT( ! radioIrq );
//        timer3_start();
//        radioIrq |= IRQ_RX_START;
//    }
//    tasklet_schedule();
//}
//
//
///**
//* Indicates PLL lock
//*/
////AVR_NONATOMIC_HANDLER(TRX24_PLL_LOCK_vect){
//ISR(TRX24_PLL_LOCK_vect, ISR_NOBLOCK){
//    RADIO_ASSERT( ! radioIrq );
//    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
//        radioIrq |= IRQ_PLL_LOCK;
//    }
//    tasklet_schedule();
//}
//
///**
//* indicates sleep/reset->trx_off mode change
//*/
////AVR_NONATOMIC_HANDLER(TRX24_AWAKE_vect){
//ISR(TRX24_AWAKE_vect, ISR_NOBLOCK){
//    RADIO_ASSERT( ! radioIrq );
//    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
//        radioIrq |= IRQ_AWAKE;
//    }
//    tasklet_schedule();
//}
//
///**
//* indicates CCA ED done
//*/
////AVR_NONATOMIC_HANDLER(TRX24_CCA_ED_DONE_vect){
//ISR(TRX24_CCA_ED_DONE_vect, ISR_NOBLOCK){
//    RADIO_ASSERT( ! radioIrq );
//    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
//        radioIrq |= IRQ_CCA_ED_DONE;
//    }
//    tasklet_schedule();
//}

//// never called, we have the RX_START interrupt instead
////async event
//void SfdCapture_fired() { }

/*----------------- TASKLET -----------------*/

//tasklet_async event
void tasklet_radio_run() {
    if( radioIrq != IRQ_NONE ) {
        serviceRadio();
    }

    if( cmd != CMD_NONE ) {
        if( cmd == CMD_DOWNLOAD ) {
            downloadMessage();
        } else if( CMD_TURNOFF <= cmd && cmd <= CMD_TURNON ) {
            changeState();
        } else if( cmd == CMD_CHANNEL ) {
            changeChannel();
        }

        if( cmd == CMD_SIGNAL_DONE ) {
            cmd = CMD_NONE;
            radioState_done();
        }
    }

    if( cmd == CMD_NONE && radio_state == STATE_RX_ON && !radioIrq ) {
        radioSend_ready();
    }
}

/*----------------- McuPower -----------------*/

//async command
 mcu_power_t McuPowerOverride_lowestState()
{
  if( (IRQ_MASK & 1<<AWAKE_EN) != 0 )
     return ATM128_POWER_EXT_STANDBY;
  else
     return ATM128_POWER_DOWN;
}

/*----------------- RadioPacket -----------------*/

////async command
// uint8_t RadioPacket_headerLength(message_t* msg)
//{
//
//return  RFA1DriverConfig_headerLength(msg) + sizeof(rfa1_header_t);
//}
//
////async command
// uint8_t RadioPacket_payloadLength(message_t* msg)
//{
//return getHeader(msg)->length - 2;
//}
//
////async command
// void RadioPacket_setPayloadLength(message_t* msg, uint8_t length)
//{
//RADIO_ASSERT( 1 <= length && length <= 125 );
//RADIO_ASSERT(  RadioPacket_headerLength(msg) + length +  RadioPacket_metadataLength(msg) <= sizeof(message_t) );
//// we add the length of the CRC, which is automatically generated
//getHeader(msg)->length = length + 2;
//}
//
////async command
// uint8_t RadioPacket_maxPayloadLength()
//{
//RADIO_ASSERT(  RFA1DriverConfig_maxPayloadLength() - sizeof(rfa1_header_t) <= 125 );
//
//return  RFA1DriverConfig_maxPayloadLength() - sizeof(rfa1_header_t);
//}
//
////async command
// uint8_t RadioPacket_metadataLength(message_t* msg)
//{
//return  RFA1DriverConfig_metadataLength(msg) + sizeof(rfa1_metadata_t);
//}
//
////async command
//void RadioPacket_clear(message_t* msg)
//{
//// all flags are automatically cleared
//}
//
///*----------------- PacketTransmitPower -----------------*/
//
////async command
//bool PacketTransmitPower_isSet(message_t* msg)
//{
//    return  TransmitPowerFlag_get(msg);
//}
//
////async command
//uint8_t PacketTransmitPower_get(message_t* msg)
//{
//    return getMeta(msg)->power;
//}
//
////async command
//void PacketTransmitPower_clear(message_t* msg)
//{
// TransmitPowerFlag_clear(msg);
//}
//
////async command
//void PacketTransmitPower_set(message_t* msg, uint8_t value)
//{
//    TransmitPowerFlag_set(msg);
//    getMeta(msg)->power = value;
//}
//
///*----------------- PacketRSSI -----------------*/
//
////async command
//bool PacketRSSI_isSet(message_t* msg)
//{
//    return RSSIFlag_get(msg);
//}
//
////async command
//uint8_t PacketRSSI_get(message_t* msg)
//{
//return getMeta(msg)->rssi;
//}
//
////async command
//void PacketRSSI_clear(message_t* msg)
//{
// RSSIFlag_clear(msg);
//}
//
////async command
//void PacketRSSI_set(message_t* msg, uint8_t value)
//{
//    // just to be safe if the user fails to clear the packet
//    TransmitPowerFlag_clear(msg);
//
//    RSSIFlag_set(msg);
//    getMeta(msg)->rssi = value;
//}
//
///*----------------- PacketTimeSyncOffset -----------------*/
//
////async command
//bool PacketTimeSyncOffset_isSet(message_t* msg)
//{
//return  TimeSyncFlag_get(msg);
//}
//
////async command
//uint8_t PacketTimeSyncOffset_get(message_t* msg)
//{
//return  RadioPacket_headerLength(msg) +  RadioPacket_payloadLength(msg) - sizeof(timesync_absolute_t);
//}
//
////async command
//void PacketTimeSyncOffset_clear(message_t* msg)
//{
// TimeSyncFlag_clear(msg);
//}
//
////async command
//void PacketTimeSyncOffset_set(message_t* msg, uint8_t value)
//{
//    // we do not store the value, the time sync field is always the last 4 bytes
//    RADIO_ASSERT(  PacketTimeSyncOffset_get(msg) == value );
//
//    TimeSyncFlag_set(msg);
//}
//
///*----------------- PacketLinkQuality -----------------*/
//
////async command
//bool PacketLinkQuality_isSet(message_t* msg)
//{
//    return TRUE;
//}
//
////async command
//uint8_t PacketLinkQuality_get(message_t* msg)
//{
//    return getMeta(msg)->lqi;
//}
//
////async command
//void PacketLinkQuality_clear(message_t* msg)
//{
//}
//
////async command
// void PacketLinkQuality_set(message_t* msg, uint8_t value)
//{
//     getMeta(msg)->lqi = value;
//}
//
///*----------------- LinkPacketMetadata -----------------*/
//
////async command
//bool LinkPacketMetadata_highChannelQuality(message_t* msg)
//{
//     return  PacketLinkQuality_get(msg) > 200;
//}

/*----------------- ExtAmpControl -----------------*/

//default //async command
// cometos_error_t ExtAmpControl_start(){
//return MAC_SUCCESS;
//}
//
//default //async command
// cometos_error_t ExtAmpControl_stop(){
//return MAC_SUCCESS;
//}
