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


#include "RPLObjectiveFunction.h"
#include "RPLRouting.h"
#include <math.h>

namespace cometos_v6 {

RPLObjectiveFunction::RPLObjectiveFunction():
            DODAGInstance(NULL),
            sequenceNumber(0)
{
}

void RPLObjectiveFunction::calculateRank(){
    if(DODAGInstance->root){
        DODAGInstance->DIO_info.rank = ROOT_DEFAULT_rank;
        return;
    }

    if(DODAGInstance->neighborhood.getPrefParentIndex() == NO_PREF_PARENT){
        DODAGInstance->DIO_info.rank = INFINITE_RANK;
    }

    //TODO Rank increase !METRIC
    DODAGInstance->DIO_info.rank = DODAGInstance->neighborhood.getPrefParent()->getRank() + (int)DODAGInstance->DIO_info.DAGConfig.minHopRankIncrease;

}


RankComp_t RPLObjectiveFunction::compareRank (uint16_t rank1, uint16_t rank2) const {

    if ((rank1/DODAGInstance->DIO_info.DAGConfig.minHopRankIncrease) > (rank2/DODAGInstance->DIO_info.DAGConfig.minHopRankIncrease)){
        return RANK_BIGGER ;
    }
    if ((rank1/DODAGInstance->DIO_info.DAGConfig.minHopRankIncrease) < (rank2/DODAGInstance->DIO_info.DAGConfig.minHopRankIncrease)){
        return RANK_SMALLER;
    }
    return RANK_EQUAL;
}


SeqComp_t RPLObjectiveFunction::compareSequence(uint8_t sequence1, uint8_t sequence2){

    //Compare Sequences wraparound - distinction between over and under SEQUENCE_TIP_VALUE (128 by default)
    if(sequence1 >= SEQUENCE_TIP_VALUE){
        if(sequence2 >= SEQUENCE_TIP_VALUE){
            //Same Area -> normal compare //TODO in small margin special compare - not implemented yet
            if(sequence1 > sequence2){
                return SEQUENCE_BIGGER;
            }
            if(sequence1 < sequence2){
                return SEQUENCE_SMALLER;
            }
            return SEQUENCE_EQUAL;
        }
        //Different area -> special compare (definitely not equal!
        //if values differ more then the sequence window
        if((uint16_t)((uint16_t)FULL_BYTE_PLUS_ONE + (uint16_t)sequence2 - (uint16_t)sequence1) <= SEQUENCE_WINDOW){
            return SEQUENCE_SMALLER;
        }
        return SEQUENCE_BIGGER;
    }
    //sequence1 < SEQUENCE_TIP_VALUE
    if(sequence2 < SEQUENCE_TIP_VALUE){
        //Same Area -> normal compare //TODO in small margin special compare - not implemented yet
        if(sequence1 > sequence2){
            return SEQUENCE_BIGGER;
        }
        if(sequence1 < sequence2){
            return SEQUENCE_SMALLER;
        }
        return SEQUENCE_EQUAL;
    }
    //different area -> special compare (definitely not equal!
    //if values differ more then the sequence window - page 65 [rfc6550]
    if((uint16_t)((uint16_t)FULL_BYTE_PLUS_ONE + (uint16_t)sequence1 - (uint16_t)sequence2) <= SEQUENCE_WINDOW){
        return SEQUENCE_BIGGER;
    }
    return SEQUENCE_SMALLER;

}

}
