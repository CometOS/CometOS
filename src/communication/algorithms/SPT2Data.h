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

#ifndef SPT2DATA_H_
#define SPT2DATA_H_

/*INCLUDES-------------------------------------------------------------------*/
#include "Layer.h"
#include "TZTypes.h"
#include "TZTCAElement.h"
#include "DistAlgoData.h"


#define SPT_MSG_TYPE_DEFAULT 168
#define SPT_MSG_PING 3
#define SPT_MSG_REMOVE 2
#define SPT_MSG_ADD 1
#define SPT_MSG_NACK 0
#define SPT_TREE_NONE 0
#define SPT_TREE_FATHER 1
#define SPT_TREE_CHILD 2

#define SPT_INVALID_ID ((node_t)-1)
#define SPT_INVALID_U8 ((uint8_t)-1)
#define SPT_INVALID_U16 ((uint16_t)-1)


/*PROTOTYPES-----------------------------------------------------------------*/

namespace cometos {

class SPT2Data : public DistAlgoData {
public:
    SPT2Data() : DistAlgoData(), pTreeState(SPT_TREE_NONE) {};

    SPT2Data(TZTCAElement* neighbor, uint8_t state = SPT_TREE_NONE,
            node_t treeID = SPT_INVALID_ID, uint16_t treeDist = SPT_INVALID_U16) :
        DistAlgoData(neighbor,0), pTreeState(state), pTreeID(treeID), pTreeDist(treeDist){}

    virtual void add(TZTCAElement* inNeighbor, uint8_t inTreeState);
    virtual void remove();

//    DistAlgoData* pData;
    // variable to store what type of neighbor it is (father, child, etc)
    uint8_t pTreeState;
    node_t pTreeID;
    uint16_t pTreeDist;
};

} // namespace cometos

#endif /* SPT2DATA_H_ */
