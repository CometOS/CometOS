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
 * @author Fabian Krome, Martin Ringwelski, Andreas Weigel
 */


#ifndef RPLOF_H_
#define RPLOF_H_

#include "types.h"
#include "cometos.h"
#include "RPLBasics.h"

namespace cometos_v6 {

enum RankComp_t : uint8_t {
    RANK_BIGGER =   1,
    RANK_EQUAL =    0,
    RANK_SMALLER =  2
};

enum SeqComp_t : uint8_t {
    SEQUENCE_BIGGER =   1,
    SEQUENCE_EQUAL =    0,
    SEQUENCE_SMALLER =  2
};

const uint8_t SEQUENCE_TIP_VALUE =  128;

const uint16_t FULL_BYTE_PLUS_ONE = 256;
const uint8_t SEQUENCE_WINDOW =     16;

class RPLObjectiveFunction;

/**
 * Abstract base class for an objective function
 *
 * Defines generic calculateRank(), compareSequence() and compareRank() methods
 * Descendants have to implement bestParent() and increasedRank()
 **/

class RPLObjectiveFunction {
public:

    RPLObjectiveFunction();
    //virtual ~RPLObjectiveFunction();

    virtual void initialize(DODAG_Object *DODAGInstance, uint8_t sequenceNumber){
        this->DODAGInstance = DODAGInstance;
        this->sequenceNumber = sequenceNumber;
    }

    //Calculate Rank based on preferred parent rank
    virtual void calculateRank();

    // Returns RANK_BIGGER if rank1 > rank2
    // Return RANK_EQUAL, RANK_SMALLER as above
    virtual RankComp_t compareRank (uint16_t rank1, uint16_t rank2) const;

    virtual RPLNeighbor * bestParent(RPLNeighbor * n1,  RPLNeighbor * n2) const = 0;

    // Returns SEQUENCE_BIGGER if sequence1 > sequence2 (wraparound handling - SEQUENCE_TIP_VALUE after which number "negativ")
    // Return SEQUENCE_EQUAL, SEQUENCE_SMALLER as above
    virtual SeqComp_t compareSequence(uint8_t sequence1, uint8_t sequence2);

    virtual void resetCurMinPathCost(){return;};

    virtual void setCurMinPathCost(){return;};

    uint16_t linkMetricRankWeight;
    uint16_t curMinPathCost;

protected:
    DODAG_Object *DODAGInstance;
    uint8_t sequenceNumber;

    // Returns the increased rank of the node because of its metrics or hop count
    // If node == NULL, it applies to self
    virtual uint16_t increasedRank(const RPLNeighbor *node = NULL) const = 0;

//    virtual uint16_t maximum(uint16_t x, uint16_t y, uint16_t z);
//    virtual uint16_t findMaximumAdvertisedRank();
//    virtual uint16_t findMaximumPathCost();
};

}

#endif /* RPLOF_H_ */
