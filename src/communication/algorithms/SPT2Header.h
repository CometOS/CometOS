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

#ifndef SPT2HEADER_H_
#define SPT2HEADER_H_


#include "DistAlgoHeader.h"
#include "Airframe.h"
#include "MacAbstractionBase.h"
#include "SPT2Data.h"


namespace cometos {

class SPT2Header : public DistAlgoHeader {
public:

    SPT2Header(uint8_t msgType = SPT_MSG_TYPE_DEFAULT,
            uint8_t state = DIST_ALGO_STATE_INIT,
            uint8_t msg = DIST_ALGO_STATE_INIT,
            node_t father = SPT_INVALID_ID,
            node_t treeID = SPT_INVALID_ID,
            uint16_t treeDist = SPT_INVALID_U16):
            DistAlgoHeader(msgType, state), pMsg(msg), pFather(father), pTreeID(treeID), pTreeDist(treeDist)
    {
        for(uint8_t i = 0; i < NEIGHBORLISTSIZE; i++) {
            pNeighbors[i] = SPT_INVALID_ID;
        }
    }

    node_t pNeighbors[NEIGHBORLISTSIZE];
    uint8_t pMsg;
    node_t pFather;
    node_t pTreeID;
    uint16_t pTreeDist;
};


/**Serialization of SPTHeader
 */
Airframe& operator<<(Airframe& frame, SPT2Header& value);

/**Deserialization of SPTHeader
 */
Airframe& operator>>(Airframe& frame, SPT2Header& value);



} /* namespace cometos */
#endif /* SPT2HEADER_H_ */
