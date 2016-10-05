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
#include "StaticConnectionManager.h"
#include "XMLParseUtil.h"
#include <FindModule.h>

using namespace omnetpp;

Define_Module(StaticConnectionManager);

void StaticConnectionManager::initialize(int stage)
{
	BaseConnectionManager::initialize(stage);
	if(stage == 1) {
		cXMLElement* cfg = par("connectionsFile");

		cXMLElementList nodeList = cfg->getElementsByTagName("node");
		for (cXMLElementList::iterator itNodes = nodeList.begin(); itNodes
				!= nodeList.end(); itNodes++) {
			node_t currSrc;
			currSrc = XMLParseUtil::getAddressFromAttribute((*itNodes), "id");

			cXMLElementList linkList = (*itNodes)->getElementsByTagName("link");
			for (cXMLElementList::iterator itLinks = linkList.begin(); itLinks
					!= linkList.end(); itLinks++) {
				node_t currDest;
				currDest = XMLParseUtil::getAddressFromAttribute((*itLinks), "dest");
				conMatrix[currSrc].insert(currDest);
//				std::cout << std::hex << "added " << currSrc << "-->" << currDest << std::dec << endl;
			}
		}
	}
}

double StaticConnectionManager::calcInterfDist()
{
	double maxLen = playgroundSize->length();

	return maxLen;
}


void StaticConnectionManager::updateConnections(int nicID, const Coord* oldPos, const Coord* newPos)
{
	updateConnectionsFromNic(nicID);
	updateConnectionsToNic(nicID);
}


void StaticConnectionManager::updateConnectionsToNic(int nicID)
{
	NicEntry* nic = nics.find(nicID)->second;
	node_t hostId = getAddr(nic);

//	printf("(To) Host: %lx\n", hostId);
	for (ConnectionsMatrix::iterator itNodes = conMatrix.begin(); itNodes != conMatrix.end(); itNodes++) {
//		printf("considering links from %x\n", itNodes->first);
		if (itNodes->second.find(hostId) != itNodes->second.end()) {
			NicEntry* otherNic = getNicFromAddress(itNodes->first);
			if (otherNic != NULL) {
//				printf("considering NIC %x with index %lx\n", otherNic->nicId, getMacAddr(otherNic));
				if ((!(otherNic->isConnected(nic))) && nic != otherNic) {
					otherNic->connectTo(nic);
//					std::cout << "Added connection from " << STREAM_HEX(getAddr(otherNic)) << " to " << STREAM_HEX(hostId) << endl;
				}
			}

		}
	}
}

void StaticConnectionManager::updateConnectionsFromNic(int nicID)
{
	NicEntry* nic = nics.find(nicID)->second;
	node_t  hostId = getAddr(nic);

//	printf("(From) Host: %lx\n", hostId);
	ConnectionsMatrix::iterator it = conMatrix.find(hostId);
	if (! (it == conMatrix.end())) {
		for (std::set<node_t >::iterator itLinks = it->second.begin(); itLinks != it->second.end(); itLinks++) {
//			printf("considering link to %x\n", *itLinks);
			NicEntry* otherNic = getNicFromAddress(*itLinks);
			if (otherNic != NULL) {
//				int otherNicAddr = getMacAddr(otherNic);
//				printf("considering NIC %x with index %x\n", otherNic->nicId, otherNicAddr);
				if ((!(nic->isConnected(otherNic))) && nic != otherNic) {
					nic->connectTo(otherNic);
//					std::cout << "Added connection from " << STREAM_HEX(hostId) << " to " << STREAM_HEX(*itLinks) << endl;
				}
			}
		}
	}
}

node_t  StaticConnectionManager::getAddr(NicEntry* nic)
{
	node_t  hostId;
	hostId = FindModule<cometos::MinAddressingBase*>::findSubModule(nic->nicPtr->getParentModule())->getShortAddr();
	return hostId;
}

NicEntry* StaticConnectionManager::getNicFromAddress(node_t  addr)
{
	for (NicEntries::iterator itNics = nics.begin(); itNics != nics.end(); itNics++) {
		node_t  currNicAddr = getAddr(itNics->second);
		if (currNicAddr == addr) {
			return itNics->second;
		}
	}
	return NULL;
}

