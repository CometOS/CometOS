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

#include "TopologyMonitor.h"

namespace cometos {
#define TOPOLOGY_MONITOR_OMNETPP_MODULE cometos::TopologyMonitor<TM_LIST_SIZE>
Define_Module( TOPOLOGY_MONITOR_OMNETPP_MODULE );

node_t operator%(const TMNodeId& lhs, const TMNodeId& rhs) {
    return lhs.id % rhs.id;
}

void serialize(ByteVector & buf, const SumsMinMax<mac_dbm_t, uint16_t, int32_t, uint32_t> & val) {
    serialize(buf, val.len);
//    serialize(buf, val.lost);
    serialize(buf, val.sum);
    serialize(buf, val.sqrSum);
    serialize(buf, val.min);
    serialize(buf, val.max);
    serialize(buf, val.numMinValues);
}

void unserialize(ByteVector & buf, SumsMinMax<mac_dbm_t, uint16_t, int32_t, uint32_t> & val) {
    unserialize(buf, val.numMinValues);
    unserialize(buf, val.max);
    unserialize(buf, val.min);
    unserialize(buf, val.sqrSum);
    unserialize(buf, val.sum);
//    unserialize(buf, val.lost);
    unserialize(buf, val.len);
}


void serialize(ByteVector & buf, const TMNodeId & val) {
    serialize(buf, val.id);
}

void unserialize(ByteVector & buf, TMNodeId & val) {
    unserialize(buf, val.id);
}

void serialize(ByteVector & buf, const TmConfig & val) {
    serialize(buf, val.beaconInterval);
    serialize(buf, val.beaconSize);
}

void unserialize(ByteVector & buf, TmConfig & val) {
    unserialize(buf, val.beaconSize);
    unserialize(buf, val.beaconInterval);
}



void serialize(ByteVector & buf, const StaticSList<TMNodeId, TM_LIST_SIZE> & list) {
    int i = 0;
    for (int it = list.begin(); it != list.end(); it = list.next(it)) {
        serialize(buf, list.get(it));
        i++;
    }
    serialize(buf, list.size());
}

void unserialize(ByteVector & buf, StaticSList<TMNodeId, TM_LIST_SIZE> & list) {
    uint8_t size;
    list.clear();
    unserialize(buf, size);
    node_t id;
    for (int i = 0; i < size && list.size() < list.max_size(); i++) {
        unserialize(buf, id);
        list.push_front(id);
    }
}

}
