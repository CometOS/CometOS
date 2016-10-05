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


#include "RPLMRHOF.h"
#include "RPLRouting.h"
#include "RPLBasics.h"
#include <math.h>

namespace cometos_v6 {

RPLMRHOF::RPLMRHOF()
{
    linkMetricRankWeight = DEFAULT_LINK_METRIC_RANK_WEIGHT;
}

RPLNeighbor *RPLMRHOF::bestParent(RPLNeighbor * n1,  RPLNeighbor * n2) const {

    if (!n1 && !n2) {
        LOG_ERROR("parent1, parent2 arguments are both NULL\n");
    }
    if (!n1) {
        return n2;
    }
    else if (!n2) {
        return n1;
    }

    uint16_t rank1 = increasedRank(n1);
    uint16_t rank2 = increasedRank(n2);

    // If rank difference is smaller than the threshold stick to the preferred parent
    uint16_t rank_diff = abs(rank1 - rank2);

    if (rank_diff < (RPL_DODAG_MC_ETX_DIVISOR / PARENT_SWITCH_THRESHOLD_DIV)) {
        if (n1 == DODAGInstance->neighborhood.getPrefParent()) {
            return n1;
        }
        else if (n2 == DODAGInstance->neighborhood.getPrefParent()) {
            return n2;
        }
    }

    if (compareRank(rank1, rank2) == RANK_BIGGER) {
        return n2;
    }
    else if (compareRank(rank1, rank2) == RANK_SMALLER) {
        return n1;
    }
    else {
        // If rank is equivalent, give preference to the preferred parent
        if (n1 == DODAGInstance->neighborhood.getPrefParent()) {
            return n1;
        }
        else {
            return n2;
        }
    }
}

void RPLMRHOF::calculateRank(){

    uint16_t ParentRank = 0;
    uint16_t NewRank = 0;
    uint16_t NewAdvertisedRank = 0;
    //uint16_t OldRank              =  DODAGInstance->DIO_info.rank;
    //uint16_t MinHopRankIncrease  =  DODAGInstance->DIO_info.DAGConfig.minHopRankIncrease;
    //uint16_t MaxRankIncrease     =  DODAGInstance->DIO_info.DAGConfig.maxRankIncrease;
    //uint16_t MinAdvRank          =  DODAGInstance->MinAdvertisedRank;

    //Rank of prefParent if present
    if (DODAGInstance->neighborhood.getPrefParent() != NULL) {
        ParentRank = DODAGInstance->neighborhood.getPrefParent()->getRank();
    }
    else {
        ParentRank = INFINITE_RANK;
    }

    curMinPathCost = ParentRank;

    //setCurMinPathCost();

    if (DODAGInstance->neighborhood.getPrefParent() == NULL) {
        NewRank = INFINITE_RANK;
    }
    else if (ParentRank == INFINITE_RANK) {
        LOG_ERROR("Parent with infinite rank\n");
    }
    else {
        NewRank = increasedRank();
        if(NewRank < (curMinPathCost + DODAGInstance->DIO_info.DAGConfig.minHopRankIncrease)) {
            NewRank = curMinPathCost + DODAGInstance->DIO_info.DAGConfig.minHopRankIncrease;
        }
    }
//     (With an exception for infinite rank), you are not allowed to have a rank greater than:
    if ((NewRank != INFINITE_RANK) && (DODAGInstance->MinAdvertisedRank != INFINITE_RANK) &&
            (compareRank(NewRank, DODAGInstance->MinAdvertisedRank + DODAGInstance->DIO_info.DAGConfig.maxRankIncrease) == RANK_BIGGER)) {
        //Rank set to Infinite because newRank is greater than minAdvertizedRank + maxRankIncrease
        NewRank = INFINITE_RANK;
    }

    // Reset minimum advertised rank
    if ((NewRank != INFINITE_RANK) && (compareRank(NewRank, DODAGInstance->MinAdvertisedRank) == RANK_SMALLER)) {
        DODAGInstance->MinAdvertisedRank = NewRank;
    }

    curMinPathCost = NewRank;

    if (DODAGInstance->neighborhood.getPrefParent() == NULL) {
        NewAdvertisedRank = INFINITE_RANK;
    }
    else {
        uint16_t MaxAdvertisedRank = findMaximumAdvertisedRank();
        uint16_t MaxPathCost = findMaximumPathCost();
        NewAdvertisedRank = maximum(curMinPathCost,MaxAdvertisedRank,MaxPathCost);
    }

    DODAGInstance->DIO_info.rank = NewAdvertisedRank;

}

uint16_t RPLMRHOF::increasedRank(const RPLNeighbor *node) const {
    uint8_t MCType = DODAGInstance->DIO_info.metricContainer.MCType;
    uint16_t OldRank;
    uint16_t MinHopRankIncrease = DODAGInstance->DIO_info.DAGConfig.minHopRankIncrease;
    uint16_t RankIncrease;
    uint16_t IncreasedRank;

    if (node) {
        OldRank = node->getRank();
    }
    else {
        OldRank = DODAGInstance->neighborhood.getPrefParent()->getRank();
    }

    if (MCType == RPL_DODAG_MC_HOPCOUNT)
    {
        // Increase by one hop
        RankIncrease = MinHopRankIncrease;
    }
    else if (MCType == RPL_DODAG_MC_ETX) {
        uint16_t metric;

        if (node) {
            metric = node->getMetric().metric.etx;
        }
        else {
            metric = DODAGInstance->neighborhood.getPrefParent()->getMetric().metric.etx;
        }
        RankIncrease = linkMetricRankWeight * metric;//This picks exactly the ETX value
    } else {
        LOG_ERROR("Unsupported MCType " << MCType << "\n");
        // TODO default value for RankIncrease
        RankIncrease=0;
        ASSERT(0);
    }

    IncreasedRank = OldRank + RankIncrease;

    // If increased rank < old rank, this can only happen due to overflow. In this case give infinite rank
    if ((OldRank != INFINITE_RANK) && (IncreasedRank != INFINITE_RANK) && (IncreasedRank < OldRank)) {
        IncreasedRank = INFINITE_RANK;
    }

    return IncreasedRank;
}

uint16_t RPLMRHOF::maximum(uint16_t x, uint16_t y, uint16_t z)
{
    uint16_t max = x;

    if (y > max) {
        max = y;
    }

    if (z > max) {
        max = z;
    }

    return max;
}

uint16_t RPLMRHOF::findMaximumAdvertisedRank()
{
    uint16_t minHopRankIncrease = DODAGInstance->DIO_info.DAGConfig.minHopRankIncrease;
    RPLNeighbor *nodeMaxAdvRank = NULL;
    for (int i=0; i<DODAGInstance->neighborhood.getSize(); i++) {
        RPLNeighbor *candidate = DODAGInstance->neighborhood.getNeighbor(i);

        if (candidate == NULL) {
            LOG_ERROR("DODAGInstance->neighborhood.getNeighbor(" << i << ") == NULL\n");
        }

        //Only search into the parent set
        if (candidate->getIsParent()) {
            //First parent is taken as the one with maxAdvRank. Next ranks are compared.
            if (!nodeMaxAdvRank) {
                nodeMaxAdvRank = candidate;
            }
            else if (candidate->getRank() > nodeMaxAdvRank->getRank()) {
                nodeMaxAdvRank = candidate;
            }
        }
    }
    if (!nodeMaxAdvRank) {
        return INFINITE_RANK;
    }
    else {
        //Return this value according to the Formula in RFC
        return minHopRankIncrease * (1 + floor(nodeMaxAdvRank->getRank()/minHopRankIncrease));
    }
}

uint16_t RPLMRHOF::findMaximumPathCost()
{
    uint16_t MaxRankIncrease = DODAGInstance->DIO_info.DAGConfig.maxRankIncrease;
    uint16_t finalValue;
    RPLNeighbor *nodeMaxAdvRank = NULL;
    for (int i=0; i<DODAGInstance->neighborhood.getSize(); i++) {
        RPLNeighbor *candidate = DODAGInstance->neighborhood.getNeighbor(i);

        if (candidate == NULL) {
            LOG_ERROR("DODAGInstance->neighborhood.getNeighbor(" << i << ") == NULL\n");
        }

        //Only search into the parent set
        if (candidate->getIsParent()) {
            //First parent is taken as the one with maxAdvRank. Next ranks are compared.
            if (!nodeMaxAdvRank) {
                nodeMaxAdvRank = candidate;
            }
            else if (increasedRank(candidate) > increasedRank(nodeMaxAdvRank)) {
                nodeMaxAdvRank = candidate;
            }
        }
    }

    if (!nodeMaxAdvRank) {
        return INFINITE_RANK;
    }
    else {
        //Return this value according to the Formula in RFC

        finalValue = nodeMaxAdvRank->getRank() - MaxRankIncrease;
        //Avoid overflow
        if (finalValue > nodeMaxAdvRank->getRank()) {
            finalValue = 0;
        }
        return finalValue;
    }

}

void RPLMRHOF::resetCurMinPathCost()
{
    curMinPathCost = 0.0;
}

void RPLMRHOF::setCurMinPathCost()
{
    if (DODAGInstance->neighborhood.getPrefParent() == NULL) {
        curMinPathCost = INFINITE_RANK;
    }
    else {
        curMinPathCost = DODAGInstance->neighborhood.getPrefParent()->getRank();
    }
}

}
