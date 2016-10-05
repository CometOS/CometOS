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
 * @author Martin Ringwelski, Andreas Weigel
 */

/*
 * lowpanconfig.h
 *
 * Assembly Mode
 *  RAM = 60 + 101 * LOWPAN_SET_ASSEMBLY_ENTRIES
 *
 *  LOWPAN_SET_BUFFER_ENTRIES = LOWPAN_SET_ASSEMBLY_ENTRIES
 *                              + LOWPAN_MAX_CONTENTS
 *                              + 4
 *  LOWPAN_SET_QUEUE_SIZE = LOWPAN_MAX_CONTENTS + 4
 *  LOWPAN_SET_ASSEMBLY_ENTRIES >= LOWPAN_MAX_CONTENTS
 *
 * Direct Mode:
 *  RAM = 52 + 18 * LOWPAN_MAX_CONTENTS
 *           + 13 * LOWPAN_SET_QUEUE_SIZE
 *           + 52 * LOWPAN_SET_ASSEMBLY_ENTRIES
 *           + 26 * LOWPAN_SET_DF_PACKETS
 *
 *  LOWPAN_SET_BUFFER_ENTRIES = LOWPAN_SET_BUFFER_SIZE / 80
 *  LOWPAN_SET_QUEUE_SIZE = LOWPAN_SET_BUFFER_ENTRIES
 *  LOWPAN_SET_ASSEMBLY_ENTRIES >= LOWPAN_MAX_CONTENTS
 *
 *  Created on: Mar 27, 2013
 *      Author: ti5mr
 */

#ifndef LOWPANCONFIG_H_
#define LOWPANCONFIG_H_

#include "MacAbstractionBase.h"

namespace cometos_v6 {

/**
 * Maximum number of fragmented packets to be assembled. Should not be higher
 * than LOWPAN_SET_BUFFER_ENTRIES.
 */
#ifndef LOWPAN_SET_ASSEMBLY_ENTRIES
    #ifdef LOWPAN_ENABLE_BIGBUFFER
        #define LOWPAN_SET_ASSEMBLY_ENTRIES 50 ///< Basestation (sim & real)
        #ifndef LOWPAN_ENABLE_DIRECT_FORWARDING
            #define LOWPAN_SET_ASSEMBLY_ENTRIES_O 10 ///< All other nodes (sim)
        #else
            #define LOWPAN_SET_ASSEMBLY_ENTRIES_O 8 ///< All other nodes (sim)
        #endif
    #else
        #ifndef LOWPAN_ENABLE_DIRECT_FORWARDING
            #define LOWPAN_SET_ASSEMBLY_ENTRIES 10 ///< testbed node
        #else
            #define LOWPAN_SET_ASSEMBLY_ENTRIES 8 ///< testbed node
        #endif
    #endif
#endif

#ifndef LOWPAN_SET_BUFFER_SIZE
    #ifdef LOWPAN_ENABLE_BIGBUFFER
        #define LOWPAN_SET_BUFFER_SIZE      65535    ///< Buffer Size for Fragments
    #else
        #define LOWPAN_SET_BUFFER_SIZE      2000    ///< Buffer Size for Fragments
    #endif
#endif


/*
 * Maximum number of assigned buffer spaces
 */
#ifndef LOWPAN_SET_BUFFER_ENTRIES
    #ifdef OMNETPP
        #define LOWPAN_SET_BUFFER_ENTRIES   255
    #else
        #ifdef LOWPAN_ENABLE_DIRECT_FORWARDING
			// to be consistent with simulation, use a number of entries
			// which is enough to theoretically fill the buffer with 64
			// byte frames
            #define LOWPAN_SET_BUFFER_ENTRIES   ((LOWPAN_SET_BUFFER_SIZE+32) / 64)
        #else
            #define LOWPAN_SET_BUFFER_ENTRIES   (2 * LOWPAN_SET_ASSEMBLY_ENTRIES + 3)
        #endif
    #endif
#endif


/**
 * Maximum number of fragmented packets to be forwarded directly.
 */
#ifndef LOWPAN_SET_DF_PACKETS
    #define LOWPAN_SET_DF_PACKETS       15
#endif

#define LOWPAN_ADDITIONAL_QUEUE_SIZE_ASSEMBLY 3

/**
 * Maximum number of entries in the queue. Any Value higher than
 * LOWPAN_SET_BUFFER_ENTRIES makes no sense.
 */
#ifndef LOWPAN_SET_QUEUE_SIZE
    #ifndef LOWPAN_ENABLE_DIRECT_FORWARDING
        #define LOWPAN_SET_QUEUE_SIZE      (LOWPAN_SET_ASSEMBLY_ENTRIES + LOWPAN_ADDITIONAL_QUEUE_SIZE_ASSEMBLY)
    #else
		#define LOWPAN_SET_QUEUE_SIZE      LOWPAN_SET_BUFFER_ENTRIES
		// I do not see any sense in setting QUEUE_SIZE for a BIG_BUFFER
		// to a fixed, potentially lower value
        //#ifndef LOWPAN_ENABLE_BIGBUFFER
            //#define LOWPAN_SET_QUEUE_SIZE      LOWPAN_SET_BUFFER_ENTRIES
        //#else
            //#define LOWPAN_SET_QUEUE_SIZE      28
        //#endif
    #endif
#endif

#define LOWPAN_MODULE_NAME "low"


/*
 * Maximum number of packets that can be send upward to the IP Layer. In
 * Assembly Mode it is also the maximum number of packets that can be forwarded.
 */
#ifndef LOWPAN_MAX_CONTENTS
    #define LOWPAN_MAX_CONTENTS        (LOWPAN_SET_ASSEMBLY_ENTRIES - 1)
#endif

const uint16_t DEFAULT_LOWPAN_TIMEOUT = 2000;
const uint16_t RTO_CLOCK_MULTIPLICATION_FACTOR = 8;
const uint16_t RTO_GRANULARITY = ((uint16_t)1 << RTO_CLOCK_MULTIPLICATION_FACTOR);//1*2^6 in this case
const uint8_t SIZE_OF_LFFR_HEADERS = 6;

// Retries is calculated by:
// SRC = LOWPAN_RC_RETRIES + (LOWPAN_RC_FACTOR * S_t / LOWPAN_RC_S_MAX)
// PRC = LOWPAN_RC_RETRIES + (LOWPAN_RC_FACTOR * S_t / S)
// ASPRC = LOWPAN_RC_RETRIES + (LOWPAN_RC_AS_FACTOR * S_t / LOWPAN_RC_S_MAX) + (LOWPAN_RC_AP_FACTOR * S_t / S)
// MSPRC = LOWPAN_RC_RETRIES + (LOWPAN_RC_FACTOR S_t * S_t) / (LOWPAN_RC_S_MAX * S)
#ifdef OMNETPP
//#warning "LOWPAN_RC_MIN_RETRIES is set to a fixed value independent of the MAC Layer"
    const uint8_t LOWPAN_RC_MIN_RETRIES = 3;   //MAC_DEFAULT_FRAME_RETRIES
#else
    const uint8_t LOWPAN_RC_MIN_RETRIES = MAC_DEFAULT_FRAME_RETRIES;
#endif

const uint8_t LOWPAN_RC_MAX_RETRIES = 15;
const uint16_t LOWPAN_RC_S_MAX =      1280;
const uint8_t LOWPAN_RC_FACTOR =      12;
const uint8_t LOWPAN_RC_AS_FACTOR =   4;
const uint8_t LOWPAN_RC_AP_FACTOR =   4;

} /* namespace cometos_v6 */

#endif /* LOWPANCONFIG_H_ */
