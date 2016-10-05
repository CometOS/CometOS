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
 * @author Andreas Weigel
 */

#include "TrafficGenCoapPollingSim.h"
#include "TrafficGen.h"
#include "XMLParseUtil.h"
#include "NeighborDiscovery.h"
#include "palId.h"

namespace cometos_v6 {

Define_Module(TrafficGenCoapPollingSim);

TrafficGenCoapPollingSim::TrafficGenCoapPollingSim() :
    TrafficGenCoapPolling()
{
}

TrafficGenCoapPollingSim::~TrafficGenCoapPollingSim() {
}

void TrafficGenCoapPollingSim::initialize() {
    TrafficGenCoapPolling::initialize();
}

void TrafficGenCoapPollingSim::finish() {
    TrafficGenCoapPolling::finish();
    collectionDurationType cd = getCollectionDurations();
    double n = cd.n();
    double sum = cd.getSum();
    double sqrsum = cd.getSqrSum();

    double collectionAvgDuration = sum / n;
    double collectionVar = TrafficGenSim::getVarFromSqrSum(n, sum, sqrsum);

    RECORD_SCALAR(collectionAvgDuration);
    RECORD_SCALAR(collectionVar);
}

IPv6Address TrafficGenCoapPollingSim::initializeTargetList() {
    omnetpp::cXMLElement* targetsFile;
    CONFIG_NED(targetsFile);
    omnetpp::cXMLElementList xmlnodes = targetsFile->getElementsByTagName(MT_NODE_ID_NAME);
    // create a list containing all network node targets
    for (omnetpp::cXMLElementList::iterator itNodes = xmlnodes.begin(); itNodes != xmlnodes.end(); itNodes++) {
           node_t currNode = XMLParseUtil::getAddressFromAttribute(*itNodes, "id");
           if(palId_id() != currNode) {
               IPv6Address addr(IP_NWK_PREFIX);
               addr.setAddressPart(currNode, 7);
               nodeList.push_front(addr);
           }
    }
    for (active = nodeList.begin(); active != nodeList.end(); active++) {
        LOG_DEBUG(nodeList.begin()->str() << "|"  << (*active).str());
    }

    // initialize destination iterator and schedule start message
    active = nodeList.begin();
    if (active != nodeList.end()) {
        return *active;
    } else {
        return IPv6Address(0,0,0,0,0,0,0,0);
    }
}

bool TrafficGenCoapPollingSim::getNextTarget(IPv6Address * addr) {
    bool wrapAround = false;
    active++;
    if (active == nodeList.end()) {
        active = nodeList.begin();
        wrapAround = true;
    }
    LOG_DEBUG("wrap=" << (uint16_t) wrapAround << "|begin=" << nodeList.begin()->str() << "|active=" << active->str());

    *addr = *active;

    return wrapAround;
}

} /* namespace cometos_v6 */
