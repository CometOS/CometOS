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


#include "RPLObjFunction0.h"
#include "RPLRouting.h"

namespace cometos_v6 {

RPLObjFunction0::RPLObjFunction0():
            DODAGInstance(NULL)
//            , sequenceNumber(0)
{
}

RPLNeighbor * RPLObjFunction0::bestParent(RPLNeighbor * n1,  RPLNeighbor * n2) const{

    if (!n1 && !n2) {
        LOG_ERROR("parent1, parent2 arguments are both NULL");
    }

    if (!n1) {
        return n2;
    }
    else if (!n2) {
        return n1;
    }
    else {
        if ((n1->getRank() == INFINITE_RANK) || (n2->getRank() == INFINITE_RANK)) {
            LOG_ERROR("Parents have infinite rank");
        }

        if (n2->getRank() > n1->getRank()) {
            return n2;
        }
        else if (n1->getRank() > n2->getRank()) {
            return n1;
        }
//        else {
//            // If rank is equivalent, give preference to the preferred parent
//            if (n1 == DODAGInstance->neighborhood.getPrefParent()) {
//                return n1;
//            }
//            else {
//                return n2;
//            }
//        }
        return n1;
    }
}

uint16_t RPLObjFunction0::increasedRank(const RPLNeighbor *node) const {
    uint16_t OldRank;
    uint16_t RankIncrease = DODAGInstance->DIO_info.DAGConfig.minHopRankIncrease;
    uint16_t IncreasedRank;

    if (node)
        OldRank = node->getRank();
    else {
        if (!DODAGInstance->neighborhood.getPrefParent())
            LOG_ERROR("Preferred parent is not set");
        OldRank = DODAGInstance->neighborhood.getPrefParent()->getRank();
        if (DODAGInstance->neighborhood.getPrefParent()->getRank() == INFINITE_RANK)
            LOG_ERROR("Rank of preferred parent is infinite");
    }

    IncreasedRank = OldRank + RankIncrease;

    // If increased rank < old rank, this can only happen due to overflow. In this case give infinite rank
    if ((OldRank != INFINITE_RANK) && (IncreasedRank != INFINITE_RANK) && (IncreasedRank < OldRank)) {
        IncreasedRank = INFINITE_RANK;
    }

    return IncreasedRank;
}

}
