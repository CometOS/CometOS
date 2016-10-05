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
#include "EmpiricModelPhy.h"
#include "EmpiricModel.h"
#include "EmpiricDecider.h"
#include "MinAddressingBase.h"
#include "XMLParseUtil.h"

#include <iostream>

using namespace std;
using namespace omnetpp;

Define_Module(EmpiricModelPhy);

AnalogueModel* EmpiricModelPhy::getAnalogueModelFromName(std::string name,
		ParameterMap& params) {

	if (name == "EmpiricModel") {
		return initializeEmpiricModel(params);
	}

	return PhyLayer::getAnalogueModelFromName(name, params);
}

AnalogueModel* EmpiricModelPhy::initializeEmpiricModel(ParameterMap& params) {

	return new EmpiricModel();
}

Decider* EmpiricModelPhy::getDeciderFromName(std::string name,
		ParameterMap& params) {
	if (name == "EmpiricDecider") {
		return initializeEmpiricDecider(params);
	}
	return PhyLayer::getDeciderFromName(name, params);
}

// creating a the link matrix for this node. iterate over all links,
// find the links for which this node is the destination and store
// the corresponding link stats
Decider* EmpiricModelPhy::initializeEmpiricDecider(ParameterMap& params) {
	cXMLElement* cfg = par("linkStatsFile");
	LinkVector linkVector;

	cXMLElementList nodeList = cfg->getElementsByTagName("node");

	cometos::MinAddressingBase* ai = FindModule<cometos::MinAddressingBase*>::findSubModule(getNode());
	ASSERT(ai != NULL);

	node_t  myAddr = ai->getShortAddr();
	for (cXMLElementList::iterator itNodes = nodeList.begin(); itNodes
			!= nodeList.end(); itNodes++) {
		node_t  currSrc;
		currSrc = XMLParseUtil::getAddressFromAttribute((*itNodes), "id");
		cXMLElementList linkList = (*itNodes)->getElementsByTagName("link");
		for (cXMLElementList::iterator itLinks = linkList.begin(); itLinks
				!= linkList.end(); itLinks++) {
			node_t  currDest;
			currDest = XMLParseUtil::getAddressFromAttribute((*itLinks), "dest");
			if (currDest == myAddr) {
				node_t currSrcNwkAddr = (node_t) currSrc;
//				node_t currSrcNwkAddr = ai->calculateNwkAddr(currSrc);
//				std::cout<< std::hex << myAddr << "|Adding linkstats from " << currSrcNwkAddr << " to " << currDest << std::dec << endl;

				// flipping the sign (config files contain rssi as positive number)!!!
				linkVector[currSrcNwkAddr].setRssiMean(
						-1 * atof((*itLinks)->getAttribute("rssiMean")));
				linkVector[currSrcNwkAddr].setRssiVar(atof(
						(*itLinks)->getAttribute("rssiVar")));
				const char* tmpChar = (*itLinks)->getAttribute("per");
				if (tmpChar != NULL) {
				    linkVector[currSrcNwkAddr].setPer(atof(tmpChar));
				} else {
				    linkVector[currSrcNwkAddr].setPer(0.0);
				}
//				std::cout << "rssi=" << linkVector[currSrcNwkAddr].getRssiMean() << endl;
			}
		}
	}

	double correctionFactorDb = par("deciderCorrectionFactor").doubleValue();

	return new EmpiricDecider(this, linkVector, par("headerLength"),
			params["sfdLength"], sensitivity, findHost()->getIndex(), correctionFactorDb, coreDebug);
}
