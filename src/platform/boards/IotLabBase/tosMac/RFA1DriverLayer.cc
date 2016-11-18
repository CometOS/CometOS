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
#include "mac_interface_ext.h"

#include <stdint.h>

#include "palLocalTime.h"
#include "cometos.h"
#include "RadioConfig.h"
#include "tasklet.h"
#include "logging.h"
#include "palExec.h"
#include "mac_constants.h"
#include "tosUtil.h"
#include "OutputStream.h"
#include "pinEventOutput.h"
#include "rf231.h"
#include "at86rf231_registers.h"
#include "Task.h"
#include "PhyCount.h"

extern cometos::PhyCounts pc;

#define RADIO_ASSERT(x) ASSERT(x)
//#define RADIO_ASSERT(x)

#ifndef MAC_DEFAULT_TX_PWR_LVL
#define MAC_DEFAULT_TX_PWR_LVL 15
#endif

static volatile bool radioStateChangeInProgress = false;
static cometos::Rf231* rf;


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

// --------------- DEFAULT IMPLEMENTATIONS OF INTERFACE FUNCTIONS --------------

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
	CMD_SENDING = 10  	// Currently sending a packet
};

enum {
	IRQ_NONE=0,
};

/**
 * Flags, if some register values have been changed. The
 * transmission to the transceiver will be delayed to a
 * suitable point of time.
 */
enum {
	SET_TX_POWER = 0x01,
	SET_CCA_MODE = 0x02,
	SET_TOS_CHANNEL = 0x04,
	SET_CCA_THRESHOLD = 0x08,
	SET_NETWORK_ID = 0x10,
	SET_NODE_ID = 0x20,
	SET_CHANNEL = 0x40
};
static uint8_t settings;



//State variables

volatile static uint8_t radio_state = STATE_TRX_OFF;
volatile static uint8_t cmd;
volatile static bool radioIrq = false;

volatile static  time_ms_t capturedTime;

//static uint8_t RX_PDT_LEVEL = 0; //Can make the receiver less sensitive ignoring incoming packages with too low rssi

/**
 * This driver duplicates the state of some transceiver
 * registers, that are often changed. This is to minimize
 * the number of SPI Transfers during use.
 */
static uint8_t txPower = 0;
static uint8_t cca_mode = 1;
static uint8_t tos_channel = 11;
static uint8_t cca_threshold = 7;
static mac_networkId_t network_id =  0xFFFF;
static mac_nodeId_t node_id = 0xFFFF;



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



/* forward declarations */
void handleInterrupt();
void downloadMessage_successCallback();
void downloadMessage_failCallback();


/*----------------- DUMMIES --------------*/
mac_result_t mac_setMode(mac_txMode_t mode) {return MAC_SUCCESS;}
mac_txMode_t mac_getMode() {return MAC_MODE_CCA | MAC_MODE_AUTO_ACK | MAC_MODE_BACKOFF;}


/*----------------- INIT -----------------*/
#include "OutputStream.h"
mac_result_t RFA1Driver_init(mac_nodeId_t myAddr, mac_networkId_t nwkId,
		mac_channel_t channel, mac_txMode_t mode, mac_ackCfg_t *ackCfg,
		mac_backoffCfg_t *backoffCfg)
{
	rf = cometos::Rf231::getInstance();
	mac_setRadioDevice(rf);

	rxMsg->data = rxBuf;
	mac_result_t result = MAC_SUCCESS;

	// outputting state via high nibble of PORTB
	EVENT_OUTPUT_INIT();

	rf->reset();
	rf->cmd_state(AT86RF231_TRX_STATE_FORCE_TRX_OFF);

	while (rf->getRfStatus() != AT86RF231_TRX_STATUS_TRX_OFF) {
		__asm("nop");
	}

	radio_state = STATE_TRX_OFF;
	rf->writeRegister(AT86RF231_REG_CCA_THRES, RFA1_CCA_THRES_VALUE);

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

	//Configure TX Power
	rf->writeRegister(AT86RF231_REG_PHY_TX_PWR, RFA1_PA_BUF_LT | RFA1_PA_LT | ( txPower & AT86RF231_PHY_TX_PWR_MASK_TX_PWR));
	radio_setTxPowerLvl(MAC_DEFAULT_TX_PWR_LVL);


	tos_channel = channel;

	//enable auto crc
	uint8_t trx_ctrl_1 = rf->readRegister(AT86RF231_REG_TRX_CTRL_1);
	trx_ctrl_1 |= AT86RF231_TRX_CTRL_1_MASK_TX_AUTO_CRC_ON;
	rf->writeRegister(AT86RF231_REG_TRX_CTRL_1, trx_ctrl_1);


	// channel is set later explicitly
	rf->writeRegister(AT86RF231_REG_PHY_CC_CCA, RFA1_CCA_MODE_VALUE);

	//    SfdCapture_setMode(ATMRFA1_CAPSC_ON);

	// instead of going to sleep, we go directly to RX mode for now
	//    SET_BIT(TRXPR,SLPTR);
	//    radio_state = STATE_SLEEP;
//	cometos::getCout() << "set node to id " << myAddr << cometos::endl;
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

	// deactivate RX_OVERRIDE feature
	rf->writeRegister(AT86RF231_REG_RX_SYN, 0);

	//radioState_turnOn();
	// enter TRX state from TRX_OFF state

	rf->cmd_state(AT86RF231_TRX_STATE_RX_ON);

	while (rf->getRfStatus() != AT86RF231_TRX_STATUS_RX_ON)
		;

	radio_state = STATE_RX_ON;

	// clear pending interrupts (automatically done by reading the interrupt status register)
	rf->readRegister(AT86RF231_REG_IRQ_STATUS);

	// Enable interrupts
	uint8_t irq_mask = AT86RF231_IRQ_STATUS_MASK_TRX_END | AT86RF231_IRQ_STATUS_MASK_RX_START | AT86RF231_IRQ_STATUS_MASK_CCA_ED_DONE;
	rf->writeRegister(AT86RF231_REG_IRQ_MASK, irq_mask);

	//Enable download frame during reception
	uint8_t ctrlReg = rf->readRegister(AT86RF231_REG_TRX_CTRL_1);
	ctrlReg |= AT86RF231_TRX_CTRL_1_MASK_RX_BL_CTRL;
	rf->writeRegister(AT86RF231_REG_TRX_CTRL_1, ctrlReg);

	rf->setInterruptCallback(&handleInterrupt);

	return MAC_SUCCESS;
}

void radio_setTxPowerLvl(mac_power_t lvl)
{
	uint8_t regValue = 0xF - (lvl & 0xF);
	if (regValue != txPower)
	{
		palExec_atomicBegin();
		settings |= SET_TX_POWER;
		txPower = regValue;
		palExec_atomicEnd();
	}

}

void radio_setCCAMode(mac_ccaMode_t ccaMode)
{
	uint8_t regValue = ccaMode << AT86RF231_PHY_CC_CCA_MODE0;

	if (regValue != ccaMode )
	{
		palExec_atomicBegin();
		settings |= SET_CCA_MODE;
		ccaMode = regValue;
		palExec_atomicEnd();
	}
}


void radio_setCCAThreshold(mac_dbm_t ccaT)
{
	uint8_t regValue = ccaT & 0xF;

	if (cca_threshold != regValue)
	{
		palExec_atomicBegin();
		settings |= SET_CCA_THRESHOLD;
		cca_threshold = regValue;
		palExec_atomicEnd();
	}
}

void radio_setNetworkId(mac_networkId_t id) {

	if (id != network_id) {
		palExec_atomicBegin();
		settings |= SET_NETWORK_ID;
		network_id = id;
		palExec_atomicEnd();
	}

}

mac_networkId_t radio_getNetworkId() {
	return network_id;
}

void radio_setNodeId(mac_nodeId_t addr) {

	if (addr != node_id) {
		palExec_atomicBegin();
		settings |= SET_NODE_ID;
		node_id = addr;
		palExec_atomicEnd();
	}
}

mac_nodeId_t radio_getNodeId() {
	return node_id;
}

/*----------------- CHANNEL -----------------*/

//tasklet_async command
uint8_t radioState_getChannel()
{
	return tos_channel;
}

//tasklet_async command
cometos_error_t radioState_setChannel(uint8_t c) {
	c &= AT86RF231_PHY_CC_CCA_MASK_CHANNEL;

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

	rf->writeRegister(AT86RF231_REG_PHY_CC_CCA, RFA1_CCA_MODE_VALUE| tos_channel);

	if( radio_state == STATE_RX_ON )
		radio_state = STATE_TRX_OFF_2_RX_ON;
	else
		cmd = CMD_SIGNAL_DONE;
}

/*----------------- TURN ON/OFF -----------------*/

inline void changeState() {
	if( (cmd == CMD_STANDBY || cmd == CMD_TURNON) && radio_state == STATE_SLEEP ) {
		RADIO_ASSERT( ! radioIrq );

		//Clear interrupt pending bits
		rf->readRegister(AT86RF231_REG_IRQ_STATUS);

		/** this is a multifunctional interrupt
		 * After P_ON, RESET or SLEEP -> AWAKE_END:
		 * else: CCA_ED_DONE
		 */
		rf->writeRegister(AT86RF231_REG_IRQ_MASK, AT86RF231_IRQ_STATUS_MASK_CCA_ED_DONE);
		rf->setSlpTr(0);

		//McuPowerState_update();
		radio_state = STATE_SLEEP_2_TRX_OFF;
		cometos::getCout() << "Awaking from sleep";

	} else if( cmd == CMD_TURNON && radio_state == STATE_TRX_OFF ) {
		RADIO_ASSERT( ! radioIrq );

		// Enable interrupts
		uint8_t irq_mask = AT86RF231_IRQ_STATUS_MASK_TRX_END | AT86RF231_IRQ_STATUS_MASK_RX_START | AT86RF231_IRQ_STATUS_MASK_CCA_ED_DONE;
		rf->writeRegister(AT86RF231_REG_IRQ_MASK, irq_mask);
		//McuPowerState_update();

		rf->cmd_state(AT86RF231_TRX_STATE_RX_ON);

		cometos::getCout() <<  "Commanded to turn RX_ON";

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

		//force to state TRX_OFF
		rf->cmd_state(AT86RF231_TRX_STATE_FORCE_TRX_OFF);

		//set irqmask to 0
		rf->writeRegister(AT86RF231_REG_IRQ_MASK, 0);

		radioIrq = IRQ_NONE;

		//        McuPowerState_update();

		radio_state = STATE_TRX_OFF;
		//        ExtAmpControl_stop();
	}

	if( cmd == CMD_TURNOFF && radio_state == STATE_TRX_OFF ) {
		rf->setSlpTr(1);

		radio_state = STATE_SLEEP;
		cmd = CMD_SIGNAL_DONE;

		cometos::getCout() << "Going to sleep";

	} else if( cmd == CMD_STANDBY && radio_state == STATE_TRX_OFF ) {
		//set irqmask to 0
		rf->writeRegister(AT86RF231_REG_IRQ_MASK, 0);
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


//tasklet_async command
mac_result_t radioSend_send(message_t* msg)
{
    // we currently assume that this method is only ever called
    // from within a tasklet run, i.e. that it will NOT be interrupted
    // by any radio transceiver interrupts

	palExec_atomicBegin();

	if( cmd != CMD_NONE || radio_state != STATE_RX_ON || radioIrq ) {
		palExec_atomicEnd();
		return MAC_ERROR_BUSY;
	}

	txMsg = msg;

	palExec_atomicEnd();

//	tasklet_schedule();

    if (settings & SET_TX_POWER)
    {
        uint8_t reg = rf->readRegister(AT86RF231_REG_PHY_TX_PWR);
        reg &= ~0x0F;
        reg |= txPower;
        rf->writeRegister(AT86RF231_REG_PHY_TX_PWR, reg);
        settings &= ~SET_TX_POWER;
    }

    rf->cmd_state(AT86RF231_TRX_STATE_PLL_ON);

    // wait for PLL_ON or RX_BUSY, which should be the only possible states
    // here, assuming that we are in RX_ON before
    uint8_t state;
    do {
        state = rf->getRfStatus();
    } while (state != AT86RF231_TRX_STATUS_PLL_ON && state != AT86RF231_TRX_STATUS_BUSY_RX);

    // RX_BUSY --> we have missed an incoming message in this short amount of time
    if( state != AT86RF231_TRX_STATUS_PLL_ON) {
        // we should then really be in BUSY_RX state
        RADIO_ASSERT( state == AT86RF231_TRX_STATUS_BUSY_RX );
        rf->cmd_state(AT86RF231_TRX_STATE_RX_ON);
        cmd = CMD_NONE;
        return MAC_ERROR_BUSY;
    }

    EVENT_OUTPUT_WRITE(PEO_RADIO_TX_START);

    txMsg->txInfo.tsData.isValid = false;

    cometos_error_t res = rf->writeFrameBuffer(txMsg->data, txMsg->phyFrameLen);

    if (res != MAC_SUCCESS) {
        rf->cmd_state(AT86RF231_TRX_STATE_RX_ON);
        // we must not call sendDone twice in a row as it will kill the CCA-Layer
        cmd = CMD_NONE;
        return MAC_ERROR_FAIL;
    }

    //The TX_Start command is given during writing of the frame buffer
    //by a rising edge of SLP_TR
    rf->cmd_state(AT86RF231_TRX_STATE_RX_ON);

    // wait for the TRX_END interrupt
    radio_state = STATE_BUSY_TX_2_RX_ON;
    cmd = CMD_TRANSMIT;
	return MAC_SUCCESS;
}

void sendMessage() {

}


/*----------------- CCA -----------------*/
//tasklet_async command
cometos_error_t radioCCA_request() {

	palExec_atomicBegin();

	if( cmd != CMD_NONE || radio_state != STATE_RX_ON ){
		palExec_atomicEnd();
		return MAC_ERROR_BUSY;
	}

	cmd = CMD_CCA;



	rf->cmd_state(AT86RF231_TRX_STATE_PLL_ON);

	cometos::PalTimer::getInstance(4)->delay(1);

	if (rf->getRfStatus() != AT86RF231_TRX_STATUS_PLL_ON) {
		cmd = CMD_NONE;
		palExec_atomicEnd();
		return MAC_ERROR_BUSY;
	}

	palExec_atomicEnd();

	// Prevent rf from switching into rx_busy while cca
	uint8_t regVal = rf->readRegister(AT86RF231_REG_RX_SYN);
	rf->writeRegister(AT86RF231_REG_RX_SYN, regVal | AT86RF231_RX_SYN_PDT_DIS_MASK);
	rf->cmd_state(AT86RF231_TRX_STATE_RX_ON);


	if (settings & SET_CCA_THRESHOLD) {
		uint8_t reg = rf->readRegister(AT86RF231_REG_CCA_THRES);
		reg &= ~0xF;
		reg |= cca_threshold;
		rf->writeRegister(AT86RF231_REG_CCA_THRES, reg);
		settings &= ~SET_CCA_THRESHOLD;
	}


	uint8_t ccaReg = rf->readRegister(AT86RF231_REG_PHY_CC_CCA);

	if (settings & SET_CCA_MODE) {
		ccaReg &= ~AT86RF231_PHY_CC_CCA_MASK_CCA_MODE;
		ccaReg |= cca_mode;
		settings &= ~SET_CCA_MODE;
	}

	rf->writeRegister(AT86RF231_REG_PHY_CC_CCA, AT86RF231_PHY_CC_CCA_MASK_CCA_REQUEST | ccaReg);

	return MAC_SUCCESS;
}

//default tasklet_async event void radioCCA_done(cometos_error_t error) { }

/*----------------- RECEIVE -----------------*/

//TODO: RX_SAFE_MODE with define
inline void downloadMessage()
{

	RADIO_ASSERT(radio_state == STATE_RX_ON && cmd == CMD_DOWNLOAD);

//	cometos_error_t result =  rf->readFrameBuffer(&rxMsg->phyFrameLen, rxMsg->data, &rxMsg->rxInfo.lqi);
//
//	if (result == COMETOS_SUCCESS)
//		downloadMessage_successCallback();
//	else
//		downloadMessage_failCallback();

	    // we should not be called
		if (rf->downloadPending())
			return;

		if (rf->downloadFinished()) {
		    rf->downloadRetrieved();
		    rxMsg->rxInfo.tsData.ts = capturedTime;
            bool sendSignal = false;

            //get rssi by ED
            uint8_t rssi = rf->readRegister(AT86RF231_REG_PHY_ED_LEVEL);
            mac_dbm_t val = -90 + rssi;
            rxMsg->rxInfo.rssi = val;

            //uint8_t link_quality;
            uint8_t crc_check_byte = rf->readRegister(AT86RF231_REG_PHY_RSSI);
            rxMsg->phyPayloadLen = rxMsg->phyFrameLen - 2;

            // rxBuffer always has to be available, we drop at higher layers
            RADIO_ASSERT(rxMsg->data != NULL);

            if ( (crc_check_byte & AT86RF231_PHY_RSSI_MASK_RX_CRC_VALID )
                    && (rxMsg->phyPayloadLen >= 3 && rxMsg->phyFrameLen - MAC_HEADER_SIZE <=  MAC_PACKET_BUFFER_SIZE)) {

                //rxMsg->rxInfo.lqi = link_quality;
                rxMsg->rxInfo.lqiIsValid = true;
                sendSignal = true;
                rxMsg->rxInfo.tsData.isValid = true;

            } else {
                pc.numCrcFail++;
                EVENT_OUTPUT_WRITE(PEO_RADIO_RX_DROPPED);
            }

            // before signalling success, reset cmd and states, otherwise, the ACK is never sent
            palExec_atomicBegin();
            radio_state = STATE_RX_ON;
            cmd = CMD_NONE;
            palExec_atomicEnd();

            // signal only if it has passed the CRC check
            if( sendSignal ) {
                pc.numRxSuccess++;
                rxMsg = radioReceive_receive(rxMsg);
            }
		} else {

            pc.numStartDl++;
            //download message while rf231 is receiving it
            mac_result_t result = rf->downloadFrameParallel(rxMsg->data, &rxMsg->phyFrameLen, &(rxMsg->rxInfo.lqi), downloadMessage_successCallback, downloadMessage_failCallback);

            if (result != MAC_SUCCESS){
                palExec_atomicBegin();
                radio_state = STATE_RX_ON;
                cmd = CMD_NONE;
                palExec_atomicEnd();
                return;
            }
		}
}

void downloadMessage_successCallback()  {
	RADIO_ASSERT(radio_state == STATE_RX_ON && cmd == CMD_DOWNLOAD);
	// we moved the actual signalling code to downloadMessage to tasklet
	// context to prevent that send can interrupt any code running in
	// tasklet context
	tasklet_schedule();
}

void downloadMessage_failCallback() {
	palLed_toggle(4);

	palExec_atomicBegin();
	radio_state = STATE_RX_ON;
	cmd = CMD_NONE;
	palExec_atomicEnd();

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

	volatile uint8_t irq;

	palExec_atomicBegin();
	radioIrq = false;
	irq = rf->readRegister(AT86RF231_REG_IRQ_STATUS);
	palExec_atomicEnd();

	rxMsg->rxInfo.rssi = RSSI_INVALID;

	if( (irq & AT86RF231_IRQ_STATUS_MASK_PLL_LOCK) != 0 ) {
		if( cmd == CMD_TURNON || cmd == CMD_CHANNEL ) {
			RADIO_ASSERT( radio_state == STATE_TRX_OFF_2_RX_ON );

			radio_state = STATE_RX_ON;
			cmd = CMD_SIGNAL_DONE;
		}
		else if( cmd == CMD_TRANSMIT ) {
			RADIO_ASSERT( radio_state == STATE_BUSY_TX_2_RX_ON );
		}
		else {
			// cometos::getCout() << radio_state << "|" << cmd << "|" << irq << cometos::endl;
			RADIO_ASSERT(false);
		}
	}


	if( cmd == CMD_TRANSMIT && (irq & AT86RF231_IRQ_STATUS_MASK_TRX_END) != 0 ) {
		EVENT_OUTPUT_WRITE(PEO_RADIO_TX_END);

		RADIO_ASSERT( radio_state == STATE_BUSY_TX_2_RX_ON );

		txMsg->txInfo.tsData.isValid = true;
		txMsg->txInfo.tsData.ts = capturedTime;
		radio_state = STATE_RX_ON;
		cmd = CMD_NONE;
		radioSend_sendDone(txMsg, MAC_SUCCESS);
	}


	if( (irq & AT86RF231_IRQ_STATUS_MASK_RX_START) != 0 ) {
	    pc.numRxStart++;
		palExec_atomicBegin();
		if( cmd == CMD_CCA ) {
			radioCCA_done(MAC_ERROR_FAIL);

			//clear the receive blocking bit
			uint8_t regVal = rf->readRegister(AT86RF231_REG_RX_SYN);
			rf->writeRegister(AT86RF231_REG_RX_SYN, regVal & ~AT86RF231_RX_SYN_PDT_DIS_MASK);

			//if at the same time an CCA_DONE Interrupt is incoming, clear the interrupt flag
			irq &= ~ AT86RF231_IRQ_STATUS_MASK_CCA_ED_DONE;
			cmd = CMD_NONE;
		}

		if (cmd == CMD_NONE) {
			EVENT_OUTPUT_WRITE(PEO_RADIO_RX_START);
			RADIO_ASSERT( radio_state == STATE_RX_ON );

			// the most likely place for busy channel and good SFD, with no other interrupts
			if( irq != AT86RF231_IRQ_STATUS_MASK_RX_START ) {
				rxMsg->rxInfo.tsData.isValid = false;
			}

			cmd = CMD_DOWNLOAD;
		}
		else {
			palExec_atomicBegin();
			if (cmd != CMD_TURNOFF) {
				//cometos::getCout() << (int) cmd << cometos::endl;
			}
			palExec_atomicEnd();
			RADIO_ASSERT( cmd == CMD_TURNOFF);
		}
		palExec_atomicEnd();
	}

	// End of reception of a frame
	if( ((irq & AT86RF231_IRQ_STATUS_MASK_TRX_END) && cmd != CMD_TRANSMIT)) {
	    pc.numRxEnd++;
		if (cmd == CMD_RECEIVE ) {
		    // is handled in
			EVENT_OUTPUT_WRITE(PEO_RADIO_RX_END);
			RADIO_ASSERT( radio_state == STATE_RX_ON );
			//cmd = CMD_DOWNLOAD;
		} else {
			// catch a situation where we are not in expectation of a reception
			// but still have received an RX_END interrupt -- reactivate
			// reception of frames
			rf->cmd_state(AT86RF231_TRX_STATE_RX_ON);
		}
	}

	//This Interrupt is a multifunctional interrupt signaling either the wakeup of the
	//rf2xx chip or the end of a clear channel assessment
	if( (irq & AT86RF231_IRQ_STATUS_MASK_CCA_ED_DONE) != 0 ) {
		if( radio_state == STATE_SLEEP_2_TRX_OFF && (cmd==CMD_STANDBY || cmd==CMD_TURNON) ) {
			radio_state = STATE_TRX_OFF;
		}
		else if( cmd == CMD_CCA ) {

			RADIO_ASSERT( radio_state == STATE_RX_ON );

			volatile uint8_t status = rf->readRegister(AT86RF231_REG_TRX_STATUS);

			//clear the receive blocking bit
			uint8_t regVal = rf->readRegister(AT86RF231_REG_RX_SYN);
			rf->writeRegister(AT86RF231_REG_RX_SYN, regVal & ~AT86RF231_RX_SYN_PDT_DIS_MASK);

			RADIO_ASSERT((status & AT86RF231_TRX_STATUS_MASK_TRX_STATUS) == AT86RF231_TRX_STATUS_BUSY_RX
					|| (status & AT86RF231_TRX_STATUS_MASK_TRX_STATUS) == AT86RF231_TRX_STATUS_RX_ON);

			cmd = CMD_NONE;

			cometos_error_t returnValue;
			if (status & AT86RF231_TRX_STATUS_MASK_CCA_DONE){
				if (status & AT86RF231_TRX_STATUS_MASK_CCA_STATUS){
					returnValue = MAC_SUCCESS;
				}
				else {
					returnValue = MAC_ERROR_BUSY;
				}
				radioCCA_done(returnValue);
			}
			else {
				radioCCA_done(MAC_ERROR_FAIL);
			}
		} else {
			RADIO_ASSERT(false);
			palLed_toggle(0x2);
		}
	}
}



void handleInterrupt(){

	RADIO_ASSERT(! radioIrq);

	palExec_atomicBegin();
	capturedTime = palLocalTime_get();
	radioIrq = true;
	palExec_atomicEnd();

	tasklet_schedule();
}


/*----------------- TASKLET -----------------*/

//tasklet_async event
void tasklet_radio_run() {

	if (radioIrq != IRQ_NONE) {
		serviceRadio();
	}


	if ((settings & SET_NODE_ID) && rf->spiAvailable()) {
		rf->writeRegister(AT86RF231_REG_SHORT_ADDR_0, 0xff & node_id);
		rf->writeRegister(AT86RF231_REG_SHORT_ADDR_1, node_id >> 8);
		settings &= ~SET_NODE_ID;
	}

	if ((settings & SET_NETWORK_ID) && rf->spiAvailable()) {
		rf->writeRegister(AT86RF231_REG_PAN_ID_0, 0xFF & network_id);
		rf->writeRegister(AT86RF231_REG_PAN_ID_1, (network_id >> 8) & 0xFF);
		settings &= ~SET_NETWORK_ID;
	}

	if( cmd != CMD_NONE ) {
		if (cmd == CMD_DOWNLOAD) {
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
	//cduint8_t irqMask = rf->readRegister(AT86RF231_REG_IRQ_MASK);

	return 0; //TODO: Necessary

	/*if( irqMask & 1<<AWAKE_EN) != 0 )
		return ATM128_POWER_EXT_STANDBY;
	else
		return ATM128_POWER_DOWN; */
}

