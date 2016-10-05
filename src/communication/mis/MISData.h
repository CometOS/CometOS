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

#ifndef MISDATA_H_
#define MISDATA_H_

/*INCLUDES-------------------------------------------------------------------*/
#include "Layer.h"
#include "TZTypes.h"
#include "TZTCAElement.h"


#define MIS_MSG_TYPE_DEFAULT 167
#define MIS_STATE_OUT 0
#define MIS_STATE_IN 1
#define MIS_INVALID_ID ((node_t)-1)
#define MIS_INVALID_U8 ((uint8_t)-1)


/*PROTOTYPES-----------------------------------------------------------------*/

namespace cometos {

class MISData{
public:
    MISData() : pNeighbor(NULL), pState(MIS_STATE_OUT) { };

    MISData(TZTCAElement* neighbor, uint8_t state = MIS_STATE_OUT) :
        pNeighbor(neighbor), pState(state){}

    void add(TZTCAElement* neighbor, uint8_t inState);
    void remove();

    TZTCAElement* pNeighbor;
    uint8_t pState;
};

} // namespace cometos

#endif /* MISDATA_H_ */
