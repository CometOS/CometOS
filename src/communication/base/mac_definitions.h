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

#ifndef MAC_DEFINITIONS_H_
#define MAC_DEFINITIONS_H_

#include "mac_constants.h"
#include "types.h"

// FIXME TODO the following is based on what the SerialComm adds to Airframe, this,
// however, is inherently bad design. The serial should add NOTHING, but instead use
// a dedicated data structure to keep track of metadata for additional requests
#ifndef SERIAL_COMM_PAYLOAD_USE
#define SERIAL_COMM_PAYLOAD_USE 5
#endif

/**
 * Size of the actual payload buffer for mac frames --- currently we work
 * around the problem, that the Airframe is used in the serial comm to store
 * header data (as opposed to the default extended operating mode mac, where
 * header data is directly written to the frame buffer */
#define MAC_PACKET_BUFFER_SIZE (MAC_MAX_PAYLOAD_SIZE + SERIAL_COMM_PAYLOAD_USE)


/** Broadcast address for nodes*/
#define MAC_BROADCAST    ((node_t)-1)


/** "Broadcast" address for network IDs */
#define MAC_NWK_BROADCAST    ((mac_networkId_t)-1)

/** Reserved value to indicate an invalid RSSI */
#define MAC_RSSI_INVALID 0

#define MAC_RSSI_MIN    -127
#define RSSI_INVALID MAC_RSSI_INVALID

#define LQI_MIN 0x0
#define LQI_MAX 0xFF

#define PHY_ED_TO_RSSI_DBM(x) (MAC_MIN_RSSI_LEVEL + (x))

/** * In this mode an acknowledge is expected for non-broadcast messages.
 * macMaxFrameRetries defines number of retries.
 */
#define TX_MODE_CCA (1<<0)

/**Performs CCA check before sending the packet
 */
#define TX_MODE_AUTO_ACK (1<<1)

/**Performs random backoff before sending a packet.
 * In case of activated CCA a maximum of
 * macMaxCSMABackoffs retries is done using
 * an exponential backoff algorithm.
 */
#define TX_MODE_BACKOFF	(1<<2)

/// type definitions //////////////////////////////////////////////////////////

typedef uint8_t lqi_t;

/*MACROS---------------------------------------------------------------------*/

/** dBm type */
typedef int8_t mac_dbm_t;

typedef uint8_t mac_power_t;

/** Short address used at the mac level */
typedef uint16_t mac_nodeId_t;


// TODO to be changed for OMNETPP together with the MacHeader format and
//      the MacAbstractionLayer (introduction of MacAck woudl be advisable),
//      which has some impact on average reliability. It used to be
//      6 bytes long (which is still not 802154-conforming); increasing
//      it to 7 bytes lead to ~ 1.5% worse simulation results for
//      RealSim_2014-11_txp51 6LoOF scenario
#ifdef OMNETPP
/** Network identifier type */
typedef uint8_t mac_networkId_t;
#else
typedef uint16_t mac_networkId_t;
#endif

/** Channel numbers */
typedef uint8_t mac_channel_t;

/** Payload size type */
typedef uint8_t mac_payloadSize_t;

/** Timestamp type TODO fixing this to a certain type makes it inflexible */
typedef time_ms_t mac_timestampMs_t;


/** Definition of return values and return value type. */
enum {
    MAC_SUCCESS = 0,
    MAC_ERROR_FAIL = 1,
    MAC_ERROR_SIZE = 2,
    MAC_ERROR_CANCEL = 3,
    MAC_ERROR_OFF = 4,
    MAC_ERROR_BUSY = 5,
    MAC_ERROR_INVALID = 6,
    MAC_ERROR_RETRY = 7,
    MAC_ERROR_ALREADY = 8,
    MAC_ERROR_NO_ACK = 9
};
/** Result type */
typedef uint8_t mac_result_t;

/** Definition of mode flag values. */
enum {
    MAC_MODE_CCA = 1,
    MAC_MODE_AUTO_ACK = 2,
    MAC_MODE_BACKOFF = 4
};
/** Mode type */
typedef uint8_t mac_txMode_t;

enum {
    MAC_CCA_MODE_ENERGY_OR_CS = 0,
    MAC_CCA_MODE_ENERGY = 1,
    MAC_CCA_MODE_CS = 2,
    MAC_CCA_MODE_ENERGY_AND_CS = 3
};
typedef uint8_t mac_ccaMode_t;


typedef struct mac_timesyncData_t {
    mac_timestampMs_t ts;
    bool isValid;
} mac_timesyncData_t;

/**
 * Contains information about signal strength and quality of a received frame.
 * LQI is provided as specified within the 802.15.4
 * standard (at least 8 distinct, uniformly distributed values in the
 * range 0x00 to 0xff, 0x00 indicating the poorest, 0xff indicating the
 * highest link quality).
 */
typedef struct mac_phyPacketInfo {
    /** RSSI for received packet, given in dB above receiver sensitivity
     *  MAC_RSSI_INVALID is used if the RSSI value is invalid if, e.g. the
     *  measurement failed */
    mac_dbm_t rssi;
    /** LQI for the received packet */
    uint8_t lqi;
    /** Indicates whether the lqi field contains a valid value */
    bool lqiIsValid;

    mac_timesyncData_t tsData;
} mac_phyPacketInfo_t;



/**
 * Contains information about a transmission request.
 */
typedef struct mac_txInfo_t {
    /** Number of backoffs performed for the corresponding frame */
    uint8_t numBackoffs;

    /** Number of frame RE-transmissions (not including initial transmission)*/
    uint8_t numRetransmissions;

    /**
     * RSSI value of a frame sent by this MAL as received at the destination
     * and reported back within an ACK frame.
     */
    mac_dbm_t remoteRssi;

    /** RSSI value of the the ack frame received by this MAL in response to
     * the data frame. SHALL be MAC_RSSI_INVALID, if sent frame was a broadcast
     * frame or the auto ack mechanism is not activated for this MAL */
    mac_dbm_t ackRssi;

    mac_timesyncData_t tsData;
} mac_txInfo_t;



/**
 * Contains parameters to configure an exponential backoff algorithm.
 *
 * @li unitBackoff
 */
typedef struct mac_backoffCfg_t {
    /** minimum backoff exponent (initial value of the BE) */
    uint8_t minBE;

    /** maximum backoff exponent used for the backoff algorithm */
    uint8_t maxBE;

    /**
     * Defines the number of retries this MAL SHALL use
     * when it is configured to perform a CCA before
     * sending and this CCA fails
     * */
    uint8_t maxBackoffRetries;

    /** duration of unit backoff period in bits; t = (unitBackoff bit)/txRate*/
    uint16_t unitBackoff;
} mac_backoffCfg_t;

/**
 * Contains parameters needed for the configuration of an acknowledgement
 * algorithm.
 */
typedef struct mac_ackCfg_t {
    /** Maximum number of frame retransmissions, before packet is discarded */
    uint8_t maxFrameRetries;
    /** Duration to wait for an ACK, before assuming a transmission error */
    uint16_t ackWaitDuration;
} mac_ackCfg_t;

namespace cometos {
#if defined SWIG || defined BOARD_python
enum macTxResult_t {
#else
enum macTxResult_t : uint8_t {
#endif
    MTR_SUCCESS,
    MTR_CHANNEL_ACCESS_FAIL,
    MTR_NO_ACK,
    MTR_INVALID
};
}


#endif /* MAC_CONSTANTS_H_ */
