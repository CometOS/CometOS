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
 * @author Gerry Siegemund
 */

#ifndef TCPWYHEADER_H_
#define TCPWYHEADER_H_


#define TZ_INVALID_ID ((node_t)-1)
#define TZ_INVALID_U8 ((uint8_t)-1)

#include "Airframe.h"
#include "MacAbstractionBase.h"
#include "TZTypes.h"

//#define LOCATION_IN_TCPWYHeader

#ifdef LOCATION_IN_TCPWYHeader
#include "palLocation.h"
#endif

namespace cometos {

class TCPWYHeader {
public:

    TCPWYHeader(uint8_t msgType = 0, uint8_t seqNum = 0) {
        this->msgType = msgType;
        this->seqNum = seqNum;
        this->ccID = TZ_INVALID_ID;
        this->ccDist = TZ_INVALID_U8;
        this->currentIndex = 0;
    }

    uint8_t msgType;
    uint8_t seqNum;
    node_t ccID;
    node_t ccDist;
    node_t neighbor[NEIGHBORLISTSIZE];
    uint8_t currentIndex;
#ifdef LOCATION_IN_TCPWYHeader
    Coordinates coordinates;
#endif
};


/**Serialization of NeighborHeader
 */
Airframe& operator<<(Airframe& frame, TCPWYHeader& value);

/**Deserialization of NeighborHeader
 */
Airframe& operator>>(Airframe& frame,TCPWYHeader& value);



} /* namespace cometos */
#endif /* TCPWYHEADER_H_ */
