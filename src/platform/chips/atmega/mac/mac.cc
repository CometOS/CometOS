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

/**
 * @author Andreas Weigel
 */

/*INCLUDES-------------------------------------------------------------------*/

#include "mac_interface.h"
#include "mac_definitions.h"
#include "atrf_hardware.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "palExec.h"
#include "palRand.h"
#include "palLocalTime.h"

#include "pinEventOutput.h"


/* VARIABLES------------------------------------------------------------------*/
/** initially false, set to true in initialization function */
static bool initialized = false;

/** the current sending mode, at the moment only with Backoff,CCA, Retries */
static mac_txMode_t txMode = 0;

/** pointer to buffer for reception of frames */
static uint8_t *rxBuffer = NULL;

/** pointer to data to be sent */
static const uint8_t *txBuffer = NULL;

/** stores stats about an active transmission */
static mac_txInfo_t txInfo;

#if MAC_DEVBOARD_MANUAL_RETRIES == 1
/** number of maximum transmissions */
static uint8_t maxNumRetransmissions = 0;
#endif

#if MAC_DEVBOARD_RESET_CCA_BE == 0
/** CSMA minimum backoff exponent */
static uint8_t initialMinBe = 3;
#endif

/** stores stats about incoming frame */
static mac_phyPacketInfo_t rxInfo;


/* Constants -----------------------------------------------------------------*/
/** Duration this mac waits for an acknowledgement */
static const uint16_t MAC_ACK_WAIT_DURATION = 592;


/* Local function prototypes -------------------------------------------------*/
static bool transmit(uint8_t const* data, mac_payloadSize_t length, mac_nodeId_t dst);

/** set the minimum backoff exponent used by the extended operating mode */
static void mac_setMinBe(uint8_t minBE);

/** get the minimum backoff exponent used by extended operating mode */
static uint8_t mac_getMinBe();

static uint8_t mac_getMaxBe();

/**
 * Set header data and copy payload into transceiver's tx buf
 * @param data   pointer to payload data
 * @param length length of payload data
 * @param dst    destination node's address
 * @return true if buffer was successfully prepared
 *         false if provided data was invalid (e.g. too large)
 */
static bool prepareTxBuffer(const uint8_t *data, uint8_t length, uint16_t dst);

/* Implementation ------------------------------------------------------------*/
mac_result_t mac_init(mac_nodeId_t myAddr, mac_networkId_t nwkId,
        mac_channel_t channel, mac_txMode_t mode, mac_ackCfg_t *ackCfg,
        mac_backoffCfg_t *backoffCfg) {
    mac_result_t result;
    initialized = false;

    // initialize CSMA-SEED with random value
    uint16_t rndVal;
#ifdef PAL_RAND
    rndVal = palRand_get();
#else
    rndVal = myAddr;
#endif
    CSMA_SEED_0 = rndVal & 0x0F;
    CSMA_SEED_1 &= ~0x07;
    CSMA_SEED_1 |= (rndVal >> 8) & 0x07;

    // reset transceiver
    TRXPR |= 1 << TRXRST;
    while (TRANSCEIVER_STATE != TRX_OFF)
        ;
    result = mac_setNodeId(myAddr);
    if (result != MAC_SUCCESS) {
        return result;
    }

    result = mac_setNetworkId(nwkId);
    if (result != MAC_SUCCESS) {
        return result;
    }

    // the following does set the CCA mode to 0
    // (Mode 3a: carrier-sense OR energy above threshold)
    PHY_CC_CCA = 0;

    result = mac_setChannel(channel);
    if (result != MAC_SUCCESS) {
        return result;
    }

    result = mac_setMode(mode);
    if (result != MAC_SUCCESS) {
        return result;
    }

    if (backoffCfg != NULL) {
        result = mac_setBackoffConfig(backoffCfg);
        if (result != MAC_SUCCESS) {
            return result;
        }
    }

    EVENT_OUTPUT_INIT();

    if (ackCfg != NULL) {
#if MAC_DEVBOARD_MANUAL_RETRIES == 1
        maxNumRetransmissions = ackCfg->maxFrameRetries;
#endif
        result = mac_setAutoAckConfig(ackCfg);
        if (result != MAC_SUCCESS) {
            return result;
        }
    }

    // clear pending interrupts
    IRQ_STATUS = 0xFF;

    // enable tx/rx end interrupt
    IRQ_MASK |= (1 << RX_END_EN) | (1 << TX_END_EN) | (1 << RX_START_EN);
#ifdef PIN_EVENT_OUTPUT
    IRQ_MASK1 |= (1 << TX_START_EN);
#endif

    // reduce ack response time to 2 symbols (higher throughput, better CCA support)
    XAH_CTRL_1 |= (1 << AACK_ACK_TIME);

    // set power to maximum value
    mac_setTxPower(mac_getMaxTxPowerLvl());

    // enter TRX state from TRX_OFF state
    TRX_STATE = CMD_PLL_ON;

    while (TRANSCEIVER_STATE != PLL_ON)
        ;

    mac_setPromiscuousMode(false);


    initialized = true;
    return MAC_SUCCESS;
}

// refer to 802.15.4 frame format of MPDU
static bool prepareTxBuffer(const uint8_t *data, uint8_t length, uint16_t dst) {

	static uint8_t sequence = 0;

    if (length > MAC_MAX_PAYLOAD_SIZE) {
        return false;
    }

	uint8_t lenTotal = length + MAC_HEADER_SIZE;

	// prepare buffer for sending
	volatile uint8_t *buffer = &TRXFBST;

	// PSDU size (length + 9 Byte header -- without PHR -- + 2 Byte CRC)
	buffer[0] = lenTotal;

	// frame control (data packet, no security, use ack, intra pan)
	if (MAC_BROADCAST == dst) {
		buffer[1] = 0x41; // no ACK request
	} else {
		buffer[1] = 0x61; // need ACK
	}

	// frame control ( use 16 bit addressing, IEEE 802.15.4-2003 compliant frame )
	buffer[2] = 0x88;

	// sequence number
	buffer[3] = sequence++;

	// only intra pan communication allowed
	buffer[4] = PAN_ID_0;
	buffer[5] = PAN_ID_1;

	// write destination
	buffer[6] = 0xff & dst;
	buffer[7] = dst >> 8;

	// write source address
	buffer[8] = SHORT_ADDR_0;
	buffer[9] = SHORT_ADDR_1;

	// write data
	memcpy((void *) &(buffer[10]), data, length);

	// CRC is appended automatically
	return true;
}


mac_result_t mac_send(uint8_t const* data, mac_payloadSize_t length,
		mac_nodeId_t dst) {
    palExec_atomicBegin();
    if (txBuffer != NULL) {
        palExec_atomicEnd();
        return MAC_ERROR_BUSY;
    }
    palExec_atomicEnd();

    txInfo.tsData.isValid = false;
    txInfo.numRetransmissions = 0;
    txInfo.numBackoffs = 0;
    txInfo.remoteRssi = RSSI_INVALID;
    txInfo.ackRssi = RSSI_INVALID;

    if (!transmit(data, length, dst)) {
        return MAC_ERROR_SIZE;
    }
	return MAC_SUCCESS;
}


static bool transmit(uint8_t const* data, mac_payloadSize_t length,
        mac_nodeId_t dst) {
    if (length > MAC_MAX_PAYLOAD_SIZE) {
        return false;
    }

    EVENT_OUTPUT_WRITE(PEO_TX_REQUEST);

//    ASSERT(txBuffer == NULL);
//    ASSERT(TRANSCEIVER_STATE != BUSY_TX_ARET);
    // TODO blocking here should be prevented, could be handled by interrupt
    txBuffer = data;
    while (TRANSCEIVER_STATE == BUSY_RX_AACK) {
        ;
    }

    // enter PLL_ON if not already there
    TRX_STATE = CMD_FORCE_PLL_ON;

    while (TRANSCEIVER_STATE != PLL_ON)
        ;

    EVENT_OUTPUT_WRITE(PEO_TX_START);
    // never move this in front of the PLL_ON state check,
    // THERE IS ONLY ONE BUFFER
    if (!prepareTxBuffer(data, length, dst)) {
        palExec_atomicBegin();
        txBuffer = NULL;
        if (rxBuffer != NULL) {
	        palExec_atomicEnd();
            while (TRANSCEIVER_STATE == STATE_TRANSITION_IN_PROGRESS) { ; }
            TRX_STATE = CMD_RX_AACK_ON;
        }
        palExec_atomicEnd();
        return false;
    }

    // use build-in csma protocol
    TRX_STATE = CMD_TX_ARET_ON;
    while (TRANSCEIVER_STATE != TX_ARET_ON)
        ;


    // start transmission
    TRX_STATE = CMD_TX_START;
    return true;
}



mac_result_t mac_setReceiveBuffer(uint8_t *buffer) {
//    ASSERT(rxBuffer == NULL);
    palExec_atomicBegin();
	rxBuffer = buffer;

	// make absolutely sure, no state transition is in progress
	while (TRANSCEIVER_STATE == STATE_TRANSITION_IN_PROGRESS) {
        ;
    }

	// if we are not in PLL_ON, some transmission (RX or TX) is still ongoing
    // and an TX_END interrupt is pending
	if (TRANSCEIVER_STATE == BUSY_TX_ARET || TRANSCEIVER_STATE == BUSY_RX_AACK) {
	    palExec_atomicEnd();
	    return MAC_SUCCESS;
	} else {

	    // just wait in case we are still switching to PLL_ON
        while (TRANSCEIVER_STATE == STATE_TRANSITION_IN_PROGRESS) { ; }

	    // in this case, we should be in PLL_ON state and if not,
	    // we accidently hit the end of TX_ARET or RX_AACK and
	    // we will try to enter it here
	    if (TRANSCEIVER_STATE != PLL_ON) {
	        TRX_STATE = PLL_ON;
	    }

	    palExec_atomicEnd();

	    // make sure we are in either PLL_ON or any reception state
	    // check for reception states because the TX_END interrupt could issue
	    // a CMD_RX_AACK_ON in between, which leads to an infinite loop
	    // when only checking for PLL_ON
	    while (TRANSCEIVER_STATE != PLL_ON
	            && TRANSCEIVER_STATE != RX_AACK_ON
	            && TRANSCEIVER_STATE != BUSY_RX_AACK)
	        ;


	    while (TRANSCEIVER_STATE == STATE_TRANSITION_IN_PROGRESS) { ; }
	    // if we are not in reception state, enter and wait for it
	    if (TRANSCEIVER_STATE != RX_AACK_ON
                && TRANSCEIVER_STATE != BUSY_RX_AACK) {
            TRX_STATE = CMD_RX_AACK_ON;

            while (TRANSCEIVER_STATE != RX_AACK_ON && TRANSCEIVER_STATE != BUSY_RX_AACK)
                ;
	    }

        return MAC_SUCCESS;
	}

}

static inline void callTxCb(mac_result_t result) {
    const uint8_t* data = txBuffer;
    txBuffer = NULL;
#if MAC_DEVBOARD_RESET_CCA_BE == 0
    mac_setMinBe(initialMinBe);
#endif
    if (data) {
        mac_cbSendDone(data, result, &txInfo);
    }
}

ISR( TRX24_TX_END_vect) {
    // check if this interrupt marks the end of a data transmission
    // (and not the transmission of an automatic ACK)
    if (TRANSCEIVER_STATE == TX_ARET_ON) {
        EVENT_OUTPUT_WRITE(PEO_RADIO_TX_END);
        // NOTE, this way of retrieving the timestamp only works for
        // unacknowledged transmission, otherwise the instant of receiveing
        // the RX_END and TX_END interrupts will differ by more than only the
        // propagation time, i.e., by the time it takes to send the ACK
        txInfo.tsData.ts = palLocalTime_get();
        txInfo.tsData.isValid = true;

        uint8_t result = TRX_STATE >> TRAC_STATUS0;
        if (result == TRAC_SUCCESS_DATA_PENDING || result == TRAC_SUCCESS) {
            callTxCb(MAC_SUCCESS);
            EVENT_OUTPUT_WRITE(PEO_TX_SUCCESS);
        } else if (result == TRAC_CHANNEL_ACCESS_FAILURE) {
            callTxCb(MAC_ERROR_BUSY);
            EVENT_OUTPUT_WRITE(PEO_TX_BO_FAIL);
        } else if (result == TRAC_INVALID) {
            callTxCb(MAC_ERROR_FAIL);
        } else if(result == TRAC_NO_ACK) {
#if MAC_DEVBOARD_MANUAL_RETRIES == 1
            // check manual retry counter
            if (txInfo.numRetransmissions >= maxNumRetransmissions) {
                callTxCb(MAC_ERROR_NO_ACK);
                EVENT_OUTPUT_WRITE(PEO_TX_FAIL);
            } else {
                txInfo.numRetransmissions++;
#if MAC_DEVBOARD_RESET_CCA_BE == 0
                uint8_t currMinBe = mac_getMinBe();
                uint8_t currMaxBe = mac_getMaxBe();
                uint8_t newMinBe = currMinBe < currMaxBe ? currMinBe + 1 : currMaxBe;
                // increase minimum backoff exponent after transmission failure
                mac_setMinBe(newMinBe);
#endif
                // start transmission and leave ISR, otherwise we might get
                // stuck later trying to get back into state RX_AACK_ON
                TRX_STATE = CMD_TX_START;
                EVENT_OUTPUT_WRITE(PEO_TX_RETRY);
                return;
            }
#else
            // in this case, all retransmissions were handled by hardware
            EVENT_OUTPUT_WRITE(PEO_TX_FAIL);
            callTxCb(MAC_ERROR_NO_ACK);
#endif
        }
    } else if (TRANSCEIVER_STATE == RX_AACK_ON) {

        // if TX_END ISR is fired after automatic ACK transmission, we assume the
        // transceiver to be in PLL_ON state, because PLL_ON is issued on
        // successful reception of a frame; settling time from RX_AACK_ON to PLL_ON
        // is given as 1 us, which should be "faster" that the interrupt delay
        // time of 16 us
        TRX_STATE = CMD_PLL_ON;
    } else if (TRANSCEIVER_STATE == PLL_ON) {
        // nothing to do here, just to catch possible states
    } else {

        // if this happens, something with the state machine is wrong.
//        ASSERT(false);
    }

    // in any case, check if we have to re-enter RX_AACK_ON ---
    // switch to idle state first, this might be superfluous, but who cares
    TRX_STATE = CMD_PLL_ON;
    if (rxBuffer != NULL) {

        // if rxBuffer is already set again, re-enter RX_AACK_ON
        while (TRANSCEIVER_STATE != PLL_ON)
            ;
        TRX_STATE = CMD_RX_AACK_ON;
    }
}


ISR( TRX24_RX_END_vect) {
    rxInfo.rssi = PHY_ED_TO_RSSI_DBM(PHY_ED_LEVEL);
    rxInfo.tsData.isValid = true;
    rxInfo.tsData.ts = palLocalTime_get();
	volatile uint8_t *buffer = &TRXFBST;

	// check frame control for supported data packets
	if ((0x41 != buffer[0] && 0x61 != buffer[0]) || 0x88 != buffer[1]) {
	    EVENT_OUTPUT_WRITE(PEO_RADIO_RX_DROPPED);
		return;
	}

	// check if CRC is valid (should be done automatically)
	if ((PHY_RSSI & (1 << RX_CRC_VALID)) == 0) {
	    EVENT_OUTPUT_WRITE(PEO_RADIO_RX_DROPPED);
		return;
	}

	// discard frame if no rxBuffer is available
	if (rxBuffer == NULL) {
	    EVENT_OUTPUT_WRITE(PEO_RADIO_RX_DROPPED);
	    return;
	}

	// extract all needed data
	uint16_t dst = buffer[5] | (buffer[6] << 8);
	uint16_t src = buffer[7] | (buffer[8] << 8);
	rxInfo.lqi = buffer[TST_RX_LENGTH];
	rxInfo.lqiIsValid = true;

	uint8_t length = TST_RX_LENGTH - MAC_HEADER_SIZE;

	// discard frame if the length register contains a size too large
	if (length > MAC_PACKET_BUFFER_SIZE) {
	    EVENT_OUTPUT_WRITE(PEO_RADIO_RX_DROPPED);
	    return;
	}

	// only set state to PLL_ON if we really call the rx callback, otherwise
    // (if no new transmission is initiated by this node) the node might starve,
    // that is, never be able to receive a new frame
	// if we do not have to send an AACK (e.g. broadcast), directly leave
	// RX_AACK_ON, otherwise schedule PLL_ON for as soon as transmission is
	// finished
	TRX_STATE = CMD_PLL_ON;
	
	uint8_t *data = rxBuffer;
	rxBuffer = NULL;

	memcpy(data, (uint8_t*) &buffer[9], length);

	EVENT_OUTPUT_WRITE(PEO_RX_DONE);
	mac_cbReceive(data, length, dst, src, mac_getNetworkId(),
			mac_getNetworkId(), &rxInfo);

	// if the reception ends here, setReceiveBuffer will be called later
	// and cause the transceiver to re-enter RX_AACK_ON
}


ISR( TRX24_RX_START_vect) {
    EVENT_OUTPUT_WRITE(PEO_RADIO_RX_START);
}

#ifdef PIN_EVENT_OUTPUT
ISR( TRX24_TX_START_vect) {
    if (TRANSCEIVER_STATE == TX_ARET_ON || TRANSCEIVER_STATE == BUSY_TX_ARET) {
        EVENT_OUTPUT_WRITE(PEO_RADIO_TX_START);
    }
}
#endif

/******************************************************************************
 * Interface implementation
 *****************************************************************************/

mac_result_t mac_sendToNetwork(uint8_t const* data, mac_payloadSize_t length,
        mac_nodeId_t dst, mac_networkId_t dstNwk) {
    // TODO
    return MAC_ERROR_FAIL;
}

mac_result_t mac_setMode(mac_txMode_t mode) {
    // currently only this mode is supported
    if (mode != (MAC_MODE_CCA | MAC_MODE_AUTO_ACK | MAC_MODE_BACKOFF)) {
        return MAC_ERROR_FAIL;
    }
    txMode = mode;
    return MAC_SUCCESS;
}

mac_txMode_t mac_getMode() {
    return txMode;
}

void mac_setPromiscuousMode(bool value) {
    if (value) {
        XAH_CTRL_1 |= (1 << AACK_PROM_MODE);
    } else {
        XAH_CTRL_1 &= ~(1 << AACK_PROM_MODE);
    }
}

bool mac_getPromiscuousMode() {
    return 1 & (XAH_CTRL_1 >> AACK_PROM_MODE);
}


mac_result_t mac_setBackoffConfig(const mac_backoffCfg_t *cfg) {
    if (cfg->maxBackoffRetries <= 7 &&
            cfg->minBE >= 0 && cfg->minBE <=8 &&
            cfg->maxBE >= cfg->minBE && cfg->maxBE <= 8) {
        uint8_t csma_retries = cfg->maxBackoffRetries;
        // set csma retries parameter
        XAH_CTRL_0 &= ~MAC_CSMA_RETRIES_MASK;
        XAH_CTRL_0 |= 0x0E & (csma_retries << 1);
        // set max backoff exponent
        CSMA_BE = (CSMA_BE & ~0xF0) | (0xF0 & (cfg->maxBE << 4));

        // store desired minimum backoff exponent in variable
#if MAC_DEVBOARD_RESET_CCA_BE == 0
        initialMinBe = cfg->minBE;
#endif
        // set min backoff exponent
        mac_setMinBe(cfg->minBE);

        return MAC_SUCCESS;
    } else {
        return MAC_ERROR_INVALID;
    }
}

void mac_getBackoffConfig(mac_backoffCfg_t *cfg) {
    cfg->minBE = CSMA_BE & 0x0F;
    cfg->maxBE = (CSMA_BE & 0xF0) >> 4;
    cfg->maxBackoffRetries = (XAH_CTRL_0 & 0x0E) >> 1;
}


mac_result_t mac_setAutoAckConfig(const mac_ackCfg_t *cfg) {
    if (cfg->maxFrameRetries >= 0 && cfg->maxFrameRetries <= 15) {
#if MAC_DEVBOARD_MANUAL_RETRIES == 1
        XAH_CTRL_0 &= ~0xF0;
#else
        XAH_CTRL_0 = (XAH_CTRL_0 & ~0xF0) | (0xF0 & (cfg->maxFrameRetries << 4));
#endif
        return MAC_SUCCESS;
    } else {
        return MAC_ERROR_INVALID;
    }

}

void mac_getAutoAckConfig(mac_ackCfg_t *cfg) {
    cfg->maxFrameRetries = (XAH_CTRL_0 & 0xF0) >> 4;
    cfg->ackWaitDuration = MAC_ACK_WAIT_DURATION;
}


inline void mac_setMinBe(uint8_t minBE) {
    CSMA_BE = (CSMA_BE & ~0x0F) | (0x0F & (minBE));
}

inline uint8_t mac_getMinBe() {
    return CSMA_BE & 0x0F;
}

inline uint8_t mac_getMaxBe() {
    return (CSMA_BE & 0xF0) >> 4;
}
