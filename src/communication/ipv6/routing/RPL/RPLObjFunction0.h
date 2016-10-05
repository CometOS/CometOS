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


#ifndef RPLOF0_H_
#define RPLOF0_H_

#include "types.h"
#include "cometos.h"
#include "RPLBasics.h"
#include "RPLObjectiveFunction.h"

namespace cometos_v6 {

/*
5. Abstract Interface to OF0

   Objective Function Zero interacts for its management and operations
   in the following ways:

   Processing DIO:  When a new DIO is received, the OF that corresponds
      to the Objective Code Point (OCP) in the DIO is triggered with the
      content of the DIO.  OF0 is identified by OCP 0 (see Section 8).

   Providing DAG Information:  The OF0 support provides an interface
      that returns information about a given instance.  This includes
      material from the DIO base header, the role (router, leaf), and
      the Rank of this node.

   Providing a Parent List:  The OF0 support provides an interface that
      returns the ordered list of the parents and feasible successors
      for a given instance to the RPL core.  This includes the material
      that is contained in the transit option for each entry.

   Triggered Updates:  The OF0 support provides events to inform it that
      a change in DAG information or Parent List has occurred.  This can
      be caused by an interaction with another system component such as
      configuration, timers, and device drivers, and the change may
      cause the RPL core to fire a new DIO or reset Trickle timers.
 *
 *
 */

class RPLObjFunction0 : public RPLObjectiveFunction {
public:

    RPLObjFunction0();

    virtual RPLNeighbor * bestParent(RPLNeighbor * n1,  RPLNeighbor * n2) const;

//    virtual void calculateRank() { RPLObjectiveFunction::calculateRank();};
//
//    virtual RankComp_t compareRank (uint16_t rank1, uint16_t rank2) const;
//    virtual SeqComp_t compareSequence(uint8_t sequence1, uint8_t sequence2);

private:

    virtual uint16_t increasedRank(const RPLNeighbor *node = NULL) const;

    DODAG_Object *DODAGInstance;
//    uint8_t sequenceNumber;
};

}

#endif /* RPLOF0_H_ */
