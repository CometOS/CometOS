/*
 * Copyright (c) 2007, Vanderbilt University
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the
 *   distribution.
 * - Neither the name of the copyright holder nor the names of
 *   its contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Miklos Maroti
 * Author: Andreas Weigel (modifications for CometOS)
 */

#include "UniqueLayer.h"


#include "tasklet.h"
#include <stdint.h>
#include "logging.h"
#include "SoftwareAckLayer.h"
#include "RFA1DriverLayer.h"
#include "RadioAlarm.h"
#include "tosUtil.h"
#include "CcaLayer.h"
#include "PacketLinkLayer.h"
#include "palId.h"

//#include <Neighborhood.h>

//generic module UniqueLayerP()
//{
//    provides
//    {
//        interface BareSend as Send;
//        interface RadioReceive;
//
//        interface Init;
//    }
//
//    uses
//    {
//        interface BareSend as SubSend;
//        interface RadioReceive as SubReceive;
//
//        interface UniqueConfig;
//        interface Neighborhood;
//        interface NeighborhoodFlag;
//    }
//}

static uint8_t uqlSequenceNumber;

//command
mac_result_t uql_init()
{
//    uqlSequenceNumber = TOS_NODE_ID << 4;
    uqlSequenceNumber = palId_id() << 4;
    return MAC_SUCCESS;
}

//command
mac_result_t uqlSend_send(message_t* msg)
{
    uniqueConfig_setSequenceNumber(msg, ++uqlSequenceNumber);
    return pllSend_send(msg);
}

//command
mac_result_t uqlSend_cancel(message_t* msg)
{
    return pllSend_cancel(msg);
}

//event
void pllSend_sendDone(message_t* msg, mac_result_t error)
{
    uqlSend_sendDone(msg, error);
}

////tasklet_async event
//bool SubReceive.header(message_t* msg)
//{
//    // we could scan here, but better be lazy
//    return signal RadioReceive.header(msg);
//}

//#define NEIGHBORHOOD_SIZE 10

//tasklet_norace
//static uint8_t receivedNumbers[NEIGHBORHOOD_SIZE];

//tasklet_async event
message_t* csmaReceive_receive(message_t* msg)
{

    // for now, we remove filtering of duplicates
//    uint8_t idx = neighborhood_insertNode(UniqueConfig.getSender(msg));
//    uint8_t dsn = uniqueConfig_getSequenceNumber(msg);
//
//    if( neighborhoodFlag_get(idx) )
//    {
//        uint8_t diff = dsn - receivedNumbers[idx];
//
//        if( diff == 0 )
//        {
//            uniqueConfig_reportChannelError();
//            return msg;
//        }
//    }
//    else
//        neighborhoodFlag_set(idx);
//
//    receivedNumbers[idx] = dsn;


    return uqlReceive_receive(msg);
}

//tasklet_async event
void neighborhood_evicted(uint8_t idx) { }



