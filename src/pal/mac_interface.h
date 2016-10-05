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
 * @author Andreas Weigel, Stefan Unterschütz, Christian Renner
 */

#ifndef MAC_INTERFACE_H_
#define MAC_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 *
 * Describes the interface of a MAC abstraction layer (MAL) capable of
 * automatic ACKs, and CSMA-CA mechanism based on
 * exponential backoffs and clear channel assessment. It may serve as a basis
 * for a network layer implementing a multi-hop routing strategy directly
 * on top of a CSMA-CA. It may also serve as a basis for other MAC layers,
 * e.g. LPL or TDMA.
 */

/*INCLUDES-------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/** include platform specific constants */
#include "mac_definitions.h"
#include "mac_constants.h"



/*PROTOTYPES-----------------------------------------------------------------*/
///////////////////////////////////////////////////////////////////////////////
// Commands   /////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/**
 * Initializes CSMA/CA mac layer. The mac layer uses 16 bit addresses for node
 * and 8 bit values for network identification. By default the maximum tx power
 * is used. The network ID is an additional filter to help distinguishing
 * different networks that are spatially co-located.
 *
 * When the initialization is done, mac_setReceiveBuffer() has to be called
 * to switch to a listen state enabling data reception.
 *
 * The transceiver SHALL be set to its maximum transmission power at
 * initialization.
 *
 * @param myAddr	 address of the node running this mac layer
 * @param nwkId		 network id
 * @param channel	 channel number
 * @param mode 		 mode of operation (if NULL default settings are used)
 * @param ackCfg	 parameters controlling the retransmission mechanism (if NULL default settings are used)
 * @param backoffCfg parameters controlling the backoff mechanism (if NULL default settings are used)
 *
 * @return   MAC_SUCCESS     if MAL was successfully initiated
 *           MAC_ERROR_SIZE  if some parameter is out of valid range
 *           MAC_ERROR_FAIL  if some non-specified error occurred
 */
mac_result_t mac_init(mac_nodeId_t myAddr,
				     mac_networkId_t nwkId,
				     mac_channel_t channel,
				     mac_txMode_t mode,
				     mac_ackCfg_t *ackCfg,
				     mac_backoffCfg_t *backoffCfg);



/**
 * Sends data to the given destination. Depending on the current txMode
 * (mac_setMode()), exponential backoffs, CCA and automatic
 * ACKs are used (or not). Ownership of the memory belonging
 * to the provided data pointer remains with the caller, but the caller
 * guarantees that the memory will not be changed until either an error
 * is returned or mac_cbSendDone() is called. mac_cbSendDone() SHALL be
 * called eventually iff this function returns MAC_SUCCESS.
 *
 * @param data		pointer to the data to send
 * @param length 	length of data, must be <= MAC_MAX_PAYLOAD_SIZE
 * @param dst		node ID of destination
 * @return
 * @li MAC_SUCCESS if this MAL is able to handle the request.
 *     If MAC_SUCCESS is returned, this MAL guarantees that
 *     the mac_cbSendDone() callback will be called eventually.
 * @li MAC_ERROR_FAIL in case some general non-specific error occurred
 * @li MAC_ERROR_SIZE if the provided length parameter
 *     is too big for this MAL
 * @li MAC_ERROR_OFF  if the MAL is not able to handle the
 *     request because it is in sleeping state
 * @li MAC_ERROR_BUSY if this MAL is currently processing
 *     another request. The caller should wait until the mac_cbSendDone()
 *     callback is called.
 * @
 */
mac_result_t mac_send(uint8_t const* data,
		              mac_payloadSize_t length,
		              mac_nodeId_t dst);


/**
 * CURRENTLY NOT USED 
 * Same as mac_send(), but additionally, the time of an event is specified.
 * If this MAL and the receiving MAL support
 * time synchronization (@see mac_supportsTimeSync), the receiving MAL
 * SHALL provide valid timesync information in its mac_cbReceive primitive.
 *
 * @copydoc mac_send(uint8_t, mac_paylaodSize_t, mac_nodeId_t)
 *
 * @param eventTime local time of an event (obtained by palLocalTime_get()), if the
 *                  receiving node supports timesync, this time will be
 *                  available at the receiver in terms of its own local time
 *
 */
//mac_result_t mac_timeSyncSend(uint8_t const* data,
//                              mac_payloadSize_t length,
//                              mac_nodeId_t dst,
//                              mac_timestampMs_t eventTime);

/**
 * Same as mac_send(), but additionally allows to send data to a network
 * with network ID different from the currently set network ID of this MAL
 * module.
 *
 * @param data		pointer to the payload data to send
 * @param length 	length of payload data (<= MAC_MAX_PAYLOAD_SIZE)
 * @param dst		destination address
 * @param dstNwk	network ID of the destination network
 * @return
 * @li MAC_SUCCESS if this MAL is able to handle the request.
 *     If MAC_SUCCESS is returned, this MAL guarantees that
 *     the mac_cbSendDone() callback will be called eventually.
 * @li MAC_ERROR_FAIL in case some general non-specific error occured
 * @li MAC_ERROR_SIZE if the provided length parameter
 *     is too big for this MAL
 * @li MAC_ERROR_OFF  if the MAL is not able to handle the
 *     request because it is in sleeping state
 * @li MAC_ERROR_BUSY if this MAL is currently processing
 *     another request. The caller should wait until the mac_cbSendDone()
 *     callback is called.
 */
mac_result_t mac_sendToNetwork(uint8_t const* data,
		                       mac_payloadSize_t length,
		                       mac_nodeId_t dst,
		                       mac_networkId_t dstNwk);


/**
 * Tells the MAL to enable the reception of frames. This command is called
 * to put the MAL into a listening state and at the same time indicate,
 * that the user of the MAL is ready to process a new incoming data frame.
 * Until this function is called, the MAL MUST NOT call the mac_cbReceive()
 * callback function and MUST NOT send any ACKs. *
 *
 * @param  buffer			 buffer which SHALL be used by the MAL to store
 *                           an incoming packet. SHALL point to a memory
 *                           area of size MAC_PACKET_BUFFER_SIZE
 * @return MAC_SUCCESS       if the transceiver was successfully put into
 *                           the listen state, the MAL takes ownership of
 *                           buffer
 *         MAC_ERROR_ALREADY if the transceiver already is in listen state,
 *                           the ownership of buffer remains with the caller
 *         MAC_ERROR_FAIL    if some other error occurred
 */
mac_result_t mac_setReceiveBuffer(uint8_t *buffer);

/**
 * Put the MAL into an active state from which it SHALL be able to handle
 * mac_send() calls and receive frames iff a reception buffer has been
 * provided to it via mac_setReceiveBuffer().
 *
 * @return MAC_SUCCESS  if the MAL was successfully put into
 *                      an active state
 *         MAC_ERROR_ALREADY if the MAL already is in an active state
 *         MAC_ERROR_BUSY    if the MAL can not switch the state at the
 *                           moment but will be able to do so later
 *         MAC_ERROR_FAIL    if some other error occurred
 */
mac_result_t mac_on();

/**
 * Tells the MAL to disable the transceiver to save energy. In sleep state,
 * no transmission requests can be handled, nor will incoming frames be
 * received. A buffer provided to the MAL via mac_setReceiveBuffer() before
 * this function is called remains valid.
 *
 * @return MAC_SUCCESS       if the transceiver was successfully put into
 *                           a sleep state
 *         MAC_ERROR_ALREADY if the transceiver already is in sleep state
 *         MAC_ERROR_FAIL    if some other error occurred
 */
mac_result_t mac_sleep();


/**
 * Get the current RSSI value from the transceiver. This is a blocking
 * call, regardless of the state the transceiver is in.
 *
 * @return RSSI value measured at the transceiver. To be interpreted as ratio
 *         of measured signal strength and transceiver sensitivity in dB.
 *         MAC_RSSI_INVALID is used to indicate that the measurement failed.
 */
mac_dbm_t mac_getRssi();


///////////////////////////////////////////////////////////////////////////////
// Callback functions - to be implemented by user of this interface ///////////
///////////////////////////////////////////////////////////////////////////////

/**
 * This callback function SHALL be called by the MAL when a packet is
 * successfully (CRC correct) received from the air interface and either
 * the MAL is set to promiscuous mode or destination address and network
 * address in the packet concur with the configuration of this MAL
 * (see mac_setNetworkId(), mac_setAddr())
 * It has to be implemented
 * by the user of the MAL. It MAY be called directly from within an ISR.
 * The ownership of the passed buffer passes to callee, that is, the
 * user of this interface. The MAL MUST NOT change the content of buffer.
 *
 * Before this function is called, the MAL SHALL be put into an rxDisable
 * state.
 * From this state it MUST NOT call the mac_cbReceive() callback function
 * again and MUST NOT send an ACK for any incoming packet.
 * The state SHALL only be left when the mac_setReceiveBuffer() function
 * is called by the user of the MAL providing a new data buffer and
 * indicating that it is ready to receive the next packet.
 *
 * In the rxDisable state, it SHOULD be possible to use this MAL to
 * send a data packet and the MAL MAY allow the transceiver to start
 * the new reception of a packet coming from its air interface. As the
 * complete reception may take some time, the upper layer is likely to
 * have called the mac_setReceiveBuffer() function before the reception
 * of the packet has finished.
 *
 * Note that mac_setReceiveBuffer() MAY be called from directly from
 * within this callback function!!!
 *
 * @param buffer	pointer to payload data
 * @param length	length of data (no longer than MAC_MAX_PAYLOAD_SIZE)
 * @param dst		destination of frame
 * @param src		source of frame
 * @param srcNwkId  ID of the origin network
 * @param dstNwkId  ID of the destination network
 * @param info      detailed information about signal strength and quality of
 *                  the received frame (see mac_phyPacketInfo_t)
 */
void mac_cbReceive(uint8_t * buffer,
                 mac_payloadSize_t length,
				 mac_nodeId_t dst,
				 mac_nodeId_t src,
				 mac_networkId_t srcNwkId,
				 mac_networkId_t dstNwkId,
				 mac_phyPacketInfo_t const * info);

/**
 * This callback SHALL be called by the MAL when a packet was received
 * from the air interface but the CRC yielded an error. It MAY be
 * called directly from within an ISR. If the MAL owns a valid reception
 * buffer, this buffer remains valid.
 */
void mac_cbDropped();


/**
 * This callback function SHALL be called by the MAL when the processing
 * of a data request initiated by the user of this MAL via mac_send() has
 * finished. The returned status code indicates success or specifies the
 * error which occurred during the transmission attempt.
 * This function MAY be called directly from within an ISR.
 *
 * @param data      pointer to the payload of the finished request
 * @param status	status of transmission, following values are possible
 * @li MAC_SUCCESS  if the transmission was successful and -- in case
 *                  ACKs are activated for this MAL -- the
 *                  packet was acknowledged
 * @li MAC_NO_ACK   if ACKs are activated, but no ACK was received after
 *                  the specified number of retries.
 * @li MAC_RETRY    if the channel could not be accessed after the specified
 *                  number of backoffs and backoff attempts
 * @li MAC_FAIL     if some other, unspecified error occurred
 * @param info      contains detailed information about the sending process
 *                  (retries etc.)
 */
void mac_cbSendDone(const uint8_t * data,
		            mac_result_t status,
		            mac_txInfo_t const * info);




///////////////////////////////////////////////////////////////////////////////
// Configuration functions ////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


/**
 * Sets the mode of the MAL. There are three flags which can be set or
 * or cleared. The MAL may support all possible combinations. It SHALL
 * support at least the following:
 * @li mode = (MAC_MODE_CCA | MAC_MODE_BACKOFF | MAC_MODE_AUTO_ACK)
 * @li mode &= ~(MAC_MODE_CCA | MAC_MODE_BACKOFF | MAC_MODE_AUTO_ACK)
 *
 * @param mode           bitfield representing the different flags
 * @li MAC_MODE_CCA      if set, the MAL SHALL use a listen-before-talk
 *  					 mechanism
 * @li MAC_MODE_BACKOFF  if set, an exponential backoff mechanism, further
 *                       specified by the mac_configureBackoffAlgorithm
 *                       function SHALL be used
 * @li MAC_MODE_AUTO_ACK if set, the MAL SHALL automatically manage sending of
 *                       ACKs for incoming packets and repeat the transmission
 *                       of outgoing packets when no corresponding ACK is
 *                       received. This retransmission mechanism SHALL only be
 *                       used for non-broadcast packets.
 */
mac_result_t mac_setMode(mac_txMode_t mode);

/**
 * Get the current mode of this layer.
 *
 * @return bit vector representing the current mode
 */
mac_txMode_t mac_getMode();


/**
 * CURRENTLY NOT USED
 * If the returning true, this MAL SHALL implement the sendTimeSync primitive
 * which enables time synchronisation between transmitter and receiver.
 *
 * If returning true, this MAL SHALL also attach timesync information to the
 * mac_cbReceive upon the reception of frames that were sent with the
 * sendTimeSync primitive.
 *
 * @return  true if this MAL supports a timesync service,
 *          false if not
 */
//bool mac_supportsTimeSync();


/**
 * Sets the filtering mode. If set to promiscuous mode,
 * the reception of every packet, for which the CRC was successful, SHALL be
 * indicated to the user of this MAL by calling mac_cbReceive(). If set to
 * standard mode, the MAL SHALL only indicate packets for which the values
 * for destination network and destination address match the configured values
 * of this MAL or for which the destination network matches and the
 * destination address is the broadcast address MAC_BROADCAST.
 *
 * @param value if set to true, the MAL SHALL enter promiscuous mode
 *              if set to false, the MAL SHALL leave the promiscuous mode
 */
void mac_setPromiscuousMode(bool value);


/**
 * Query if this MAL is in promiscuous mode.
 *
 * @return true  if MAL is in promiscuous mode
 *         false else
 */
bool mac_getPromiscuousMode();


/**
 * Set the transmission power level this MAL uses to transmit packets.
 * @param pwrLevel transmission power level. The MAL SHALL define a number of
 *                 distinct power levels between mac_getMinTxPowerLvl() and
 *                 mac_getMaxTxPowerLvl() (inclusively). Higher values
 *                 correspond to higher output power of the used
 *                 transceiver.
 *
 * @return MAC_SUCCESS    if the txPwrLevel was successfully set
 *         MAC_ERROR_SIZE if the given value is out of the allowed range
 */
mac_result_t mac_setTxPower(mac_power_t pwrLevel);


/**
 * Returns the minimum value for transmission power of this MAL
 * @return Min. txPower value
 */
mac_power_t mac_getMinTxPowerLvl();

/**
 * Return the maximum value for transmission power of this MAL
 * @return Max. txPower value
 */
mac_power_t mac_getMaxTxPowerLvl();



/**
 * Get the currently set transmission power. Allowed values
 * range from 0 to MAC_MAX_TX_POWER_LEVEL
 *
 * @return The transmission power level this MAL currently uses.
 */
mac_power_t mac_getTxPower();


/**
 * Enable or disable an external LNA.
 * @param enable	True to enable the LNA
 *
 * @return MAC_SUCCESS  if the state was successfully changed
 */
mac_result_t mac_setRxLnaState(bool enable);


/**
 * Enable antenna diversity.
 *
 * @return MAC_SUCCESS  if the state was successfully changed
 */
mac_result_t mac_enableAutomaticDiversity();

/**
 * Disable antenna diversity.
 * @param selected_antenna	Antenna to use
 *
 * @return MAC_SUCCESS  if the state was successfully changed
 */
mac_result_t mac_disableAutomaticDiversity(uint8_t selected_antenna);

/**
 * Sets the transceiver to the given channel.
 * Channel Ids SHALL be ordered ascending corresponding to their
 * center frequencies. The valid range of channels is defined
 * by MAC_MIN_CHANNEL_NUMBER and MAC_MAX_CHANNEL_NUMBER.
 *
 * @param channelNumber number of the channel to tune the transceiver to
 *                      valid numbers range from 1 to mac_getNumChannels()
 * @return MAC_SUCCESS  if the channel was successfully changed
 *         MAC_INVALID  if the given channelNumber is not supported
 */
mac_result_t mac_setChannel(mac_channel_t channelNumber);


/**
 * Get the channel the transceiver is currently set to.
 *
 * @return channel number used by the transceiver
 */
mac_channel_t mac_getChannel();

/**
 * Set the MAL to a new network ID.
 *
 * @param id  new network ID
 * @return MAC_SUCCESS if the address was set successfully
 *         MAC_SIZE    if the given parameter is not with the valid range
 */
mac_result_t mac_setNetworkId(mac_networkId_t id);


/**
 * Get the current network ID.
 *
 * @return network ID this MAL is currently using
 */
mac_networkId_t mac_getNetworkId();



/**
 * Set this MAL's MAC address. Addresses from 0 to 0xFFFE are allowed,
 * 0xFFFF is reserved for broadcast messages.
 *
 * @param addr  MAC address
 * @return MAC_SUCCESS if the address was set successfully
 *         MAC_SIZE    if the given parameter is not with the valid range
 */
mac_result_t mac_setNodeId(mac_nodeId_t addr);


/**
 * Get the current MAC address of this MAL
 *
 * @return the MAC address
 */
mac_nodeId_t mac_getNodeId();



/**
 * Configures the backoff algorithm which is used if the MAL is
 * set to the corresponding mode. The backoff algorithm randomly
 * chooses a value between (0 and (2^minBe) - 1) * unitBackoff
 * in microseconds for its first backoff. If CCA is enabled, the MAL
 * performs a CCA; if it fails, the backoff exponent is increased by one
 * (up to a maximum of maxBE), and a new backoff is performed. At most
 * maxBackoffRetries retries are executed, when the CCA fails consecutively.
 *
 * @param cfg  struct containing the configuration parameters
 */
mac_result_t mac_setBackoffConfig(const mac_backoffCfg_t * cfg);

/**
 * Get the current backoff configuration structure
 */
void mac_getBackoffConfig(mac_backoffCfg_t * cfg);


/**
 * Set the CCA mode to one of the following:
 * @li MAC_CCA_MODE_ENERGY_OR_CS: CCA reports busy medium, if energy above thres-
 *                                hold is sensed OR a complying signal is
 *                                detected
 * @li MAC_CCA_MODE_ENERGY: CCA reports a busy medium, if energy above
 *                          specified threshold (@see mac_setCCAThreshold)
 *                          is detected
 * @li MAC_CCA_MODE_CS:     CCA reports a busy medium, if a complying signal
 *                          is detected
 * @lis MAC_CCA_MODE_ENERGY_AND_CS: CCA reports a busy medium, if a complying
 *                                  signal AND energy above specified threshold
 *                                  is detected
 * @param mode CCA mode to set this mac to
 * @return MAC_SUCCESS if mode was successfully changed
 *         MAC_ERROR_INVALID if given parameter was out of range
 */
mac_result_t mac_setCCAMode(mac_ccaMode_t mode);

/**
 * Return the currently active CCA mode
 *
 * @return CCA mode
 */
mac_ccaMode_t mac_getCCAMode();

/**
 * Set the threshold above which the CCA mechanism considers the channel
 * occupied. It is provided as signal strength in dBm.
 *
 * @param  threshold signal strength in dBm
 * @return MAC_SUCCESS     if the new threshold was set as required
 *         MAC_ERROR_SIZE  if the parameter is out of the allowed range
 */
mac_result_t mac_setCCAThreshold(mac_dbm_t threshold);

/**
 * Get the currently used CCA threshold.
 *
 * @return value of the current CCA threshold to receiver sensitivity in dB
 */
mac_dbm_t mac_getCCAThreshold();

/**
 * Set the configuration parameters related to the frame retransmission
 * mechanism.
 *
 * @param cfg  Configuration parameter structure
 */
mac_result_t mac_setAutoAckConfig(const mac_ackCfg_t *cfg);


/**
 * Get the currently used configuration parameters
 *
 * @return  Configuration parameter structure
 */
void mac_getAutoAckConfig(mac_ackCfg_t *cfg);



#ifdef __cplusplus
}
#endif

#endif /* MAC_H_ */
