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
#include "StaticLocation.h"
#include "MinAddressingBase.h"
#include "XMLParseUtil.h"

using namespace omnetpp;

Define_Module(StaticLocation);

void StaticLocation::initialize(int stage) {
	if (stage == 0) {
		BaseMobility::initialize(stage);
	} else if (stage == 1) {
		cometos::MinAddressingBase* addr = FindModule<cometos::MinAddressingBase*>::findSubModule(getNode());
		ASSERT(addr != NULL);
		node_t myAddr = addr->getShortAddr();

		cXMLElement* cfg = par("locationFile");
		if (cfg == NULL) {
			error("No location file specified");
		}
		cXMLElementList hostList = cfg->getElementsByTagName("node");

		for (cXMLElementList::iterator itNodes = hostList.begin(); itNodes
					!= hostList.end(); itNodes++) {
			node_t currNode = XMLParseUtil::getAddressFromAttribute((*itNodes), "id");
			if (currNode == myAddr) {
				world = FindModule<BaseWorldUtility*>::findGlobalModule();
				if (world == NULL) {
					error("Could not find BaseWorldUtility module");
				}
				Coord pos = world->getRandomPosition();
				bool use2D = world->use2D();

				double x = XMLParseUtil::getDoubleFromAttribute((*itNodes), "x");
				double y = XMLParseUtil::getDoubleFromAttribute((*itNodes), "y");
				double z = XMLParseUtil::getDoubleFromAttribute((*itNodes), "z");
				if (x > -1) { pos.setX(x); }
				if (y > -1) { pos.setY(y); }
				if (!use2D && z > -1) { pos.setZ(z); }

				// set start-position and start-time (i.e. current simulation-time) of the Move
				move.setStart(pos);

				//check whether position is within the playground
				if (!move.getStartPos().isInRectangle(Coord(use2D), world->getPgs())) {
					error("node position specified in omnetpp.ini exceeds playgroundsize");
				}

				// set speed and direction of the Move
				move.setSpeed(0);
				move.setDirectionByVector(Coord(use2D));

				//get BBItem category for Move
				moveCategory = utility->getCategory(&move);
			}
		}

		// finally let BaseMobility update the GUI etc.
		BaseMobility::initialize(stage);
	}
}
