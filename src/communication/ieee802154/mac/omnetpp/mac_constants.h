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

/*
 * Contains platform-specific defines for the
 * mac abstraction layer
 *
 * @author Andreas Weigel
 */

#ifndef MAC_CONSTANTS_H_
#define MAC_CONSTANTS_H_

/**
 * Size of payload allowed by omnetpp mac layer
 */
#define MAC_MAX_PAYLOAD_SIZE 116

/**
 * Maximum power level the transceiver used by this MAL supports; the
 * maximum power level SHALL correspond to the maximum actual transmission power
 */
#define MAC_MAX_TX_POWER_LEVEL 15

/**
 * Sensitivity of the transceiver. Often defined as the incoming signal power
 * (at antenna terminal) for which the packet error rate of 20 byte packets
 * (size of PHY payload) is less than 1% (no interference present).
 */
#define MAC_TRANSCEIVER_SENSITIVITY 100


#define MAC_DEFAULT_CCA_MODE 0

#define MAC_DEFAULT_CCA_THRESHOLD -90

#define MAC_DEFAULT_CHANNEL 12

#define MAC_DEFAULT_MAX_CCA_RETRIES 5

#define MAC_DEFAULT_NWK_ID 0

/** Lowest usable channel number*/
#define MAC_MIN_CHANNEL 11

/** Highest usable channel number */
#define MAC_MAX_CHANNEL 26

/** default number of frame retries */
#define MAC_DEFAULT_FRAME_RETRIES 3

/** default duration to wait for an ACK before frame is assumed lost, in milliseconds */
#define MAC_DEFAULT_ACK_WAIT_DURATION 700

/** default minimum backoff exponent */
#define MAC_DEFAULT_MIN_BE 3

/** dummy parameter for power lvl used on hardware nodes */
#define MAC_DEFAULT_TX_POWER_LVL 0

/** default maximum backoff exponent */
#define MAC_DEFAULT_MAX_BE 8

/** default unit backoff, in microseconds */
#define MAC_DEFAULT_UNIT_BACKOFF 320

#endif /* MAC_CONSTANTS_H_ */
