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
 * @author Stefan Unterschütz
 */

#include "Mobility.h"
#include <iostream>
#include "BaseModule.h"
#include "logging.h"

using namespace std;
using namespace omnetpp;

Define_Module( Mobility);

bool Mobility::isInitialized = false;
std::map<int, Mobility::node_info_t> Mobility::properties;
std::map<node_t, int> Mobility::idToindex;

Mobility::node_info_t& Mobility::getNodeProperties(node_t id) {
	return properties[idToindex[id]];
}

void Mobility::initialize(int stage) {

	BaseModule::initialize(stage);

	if (stage == 0) {
		hasPar("coreDebug") ?
				coreDebug = par("coreDebug").boolValue() : coreDebug = false;

		coreEV << "initializing BaseMobility stage " << stage << endl;

		// get utility pointers (world and host)
		world = FindModule<BaseWorldUtility*>::findGlobalModule();
		if (world == NULL
			)
			error("Could not find BaseWorldUtility module");

		coreEV << "initializing BaseUtility stage " << stage << endl; // for node position
		//get a pointer to the host
		hostPtr = findHost();
		hostId = hostPtr->getId();

		updateInterval = 0;

		// initialize Move parameter
		bool use2D = world->use2D();

		//initalize position with random values
		Coord pos = world->getRandomPosition();

		// load topology from file
		if (isInitialized == false) {

			cXMLElement *a = par("topo");
			cXMLElementList list = a->getElementsByTagName("node");
			int index = 0;
			for (cXMLElementList::iterator it = list.begin(); it != list.end();
					it++) {

				// read and store node properties
				int id;
				sscanf((*it)->getAttribute("id"), "%x", &id);
				properties[index].id = id;
				idToindex[id] = index;
				properties[index].x = atof((*it)->getAttribute("x"));
				properties[index].y = atof((*it)->getAttribute("y"));

				const char* isDom = (*it)->getAttribute("isDominator");
				if (isDom) {
					properties[index].dom = atoi(isDom);
				} else {
					properties[index].dom = false;
				}

				cXMLElementList nbs = (*it)->getElementsByTagName("link");
				for (cXMLElementList::iterator it2 = nbs.begin();
						it2 != nbs.end(); it2++) {
					int nbid;
					sscanf((*it2)->getAttribute("id"), "%x", &nbid);
					properties[index].nbs.push_back(nbid);
				}

				index++;

			}
			isInitialized = true;
		}

		// update this node
		int index = getParentModule()->getIndex();
		ASSERT(properties.count(index) != 0);

		// store id
		//	cout<<" Write "<<(int)index<< "  id ="<<location[index].id<<endl;
		getParentModule()->par("id") = properties[index].id;
		par("x") = (int) properties[index].x;
		par("y") = (int) properties[index].y;

		//getParentModule()->par("x") = (int) properties[index].x;
		//getParentModule()->par("y") = (int) properties[index].y;

		if (getParentModule()->hasPar("dom")) {
			getParentModule()->par("dom") = (bool) properties[index].dom;
			//if (getParentModule()->par("dom")) {
			//	getParentModule()->getDisplayString().setTagArg("i", 1, "red");
			//}

		}

		//read coordinates from parameters if available
		pos.setX(properties[index].x);
		pos.setY(properties[index].y);
		pos.setZ(par("z"));
		LOG_INFO(
				"mobility index " << properties[index].id << " id "
						<< properties[index].id << " x " << properties[index].x
						<< " y " << properties[index].y);
		/*
		 printf("set position of %x to (%f, %f)\n", location[index].id,
		 location[index].x, location[index].y);
		 */
		// set start-position and start-time (i.e. current simulation-time) of the Move
		move.setStart(pos);
		coreEV << "start pos: " << move.getStartPos().info() << endl;

		//check whether position is within the playground
		if (!move.getStartPos().isInRectangle(Coord(use2D), world->getPgs())) {
			error(
					"node position specified in omnetpp.ini exceeds playgroundsize");
		}

		// set speed and direction of the Move
		move.setSpeed(0);
		move.setDirectionByVector(Coord(use2D));

		//get BBItem category for Move
		moveCategory = utility->getCategory(&move);

	} else if (stage == 1) {

		coreEV << "initializing BaseMobility stage " << stage << endl;
		// print new host position on the screen and update bb info
		updatePosition();
		if (move.getSpeed() > 0 && updateInterval > 0) {
			ASSERT(false);
		}

	}

}
