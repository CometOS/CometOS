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
 * @author Bastian Weigelt
 */

#ifndef TCPWYCONTROLDATA_H_
#define TCPWYCONTORLDATA_H_


#include "Airframe.h"
#include "MacAbstractionBase.h"
#include "TZTypes.h"

#define TCPWY_CONTROL_PING 0
#define TCPWY_CONTROL_ADD 1
#define TCPWY_CONTROL_REMOVE 2
#define TCPWY_CONTROL_ACTIVATE 3
#define TCPWY_CONTROL_RECEIVED_NEIGHBOR 10
#define TCPWY_CONTROL_RECEIVED 11
#define TCPWY_CONTROL_SEND 12

namespace cometos {

class TCPWYControlHeader {
public:

    TCPWYControlHeader(uint8_t controlType = TCPWY_CONTROL_PING, node_t targetId = 0) :
                pMsgType(TCPWY_MSG_TYPE_CTRL),
                pControlType(controlType),
                pTargetId(targetId) {}

    uint8_t pMsgType;
    uint8_t pControlType;
    node_t pTargetId;
};


/**Serialization of ControlHeader
 */
Airframe& operator<<(Airframe& frame, TCPWYControlHeader& value);

/**Deserialization of ControlHeader
 */
Airframe& operator>>(Airframe& frame,TCPWYControlHeader& value);



} /* namespace cometos */
#endif /* TCPWYHEADER_H_ */
