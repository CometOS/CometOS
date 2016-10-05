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
#ifndef __EMPIRICMODEL_STATICCONNECTIONMANAGER_H_
#define __EMPIRICMODEL_STATICCONNECTIONMANAGER_H_

#include <omnetpp.h>
#include <BaseConnectionManager.h>
#include "MinAddressingBase.h"

/**
 * This connection manager connects different NICs (network interface cards)
 * corresponding to a configuration file. The connections are initialized
 * at the beginning and remain fixed, i.e. there is no reaction on positioning
 * changes.
 */
class StaticConnectionManager : public BaseConnectionManager
{
	typedef std::map<node_t, std::set<node_t> > ConnectionsMatrix;
protected:
	ConnectionsMatrix conMatrix;

	cometos::MinAddressingBase * ai;

protected:

	/**
	 * @Override
	 * Initialize
	 */
	virtual void initialize(int stage);

	/**
	 * @Override
	 * Returns a distance equal to the max. distance between the two
	 * nodes farthest away. With static links, distance is neither
	 * known nor matters for interference calculations.
	 */
	virtual double calcInterfDist();

	/**
	 * @Override
	 * Updates the connections of the NIC with the given ID. The passed position
	 * parameters are ignored completely, as with static links mobility does not
	 * make any sense. Called when a new NIC is registered with this
	 * ConnectionManager.
	 *
	 * @param nicID ID of the NIC which connections are to be updated
	 * @param oldPos inherited, unused -- this is a STATIC connection manager
	 * @param newPos inherited, unused -- this is a STATIC connection manager
	 */
	virtual void updateConnections(int nicID, const Coord* oldPos, const Coord* newPos);

private:

	/**
	 * Creates connections destined to the NIC with the given nicID from all
	 * other NICs.
	 * @param nicID ID of the NIC to create incoming connections to
	 */
	void updateConnectionsToNic(int nicID);

	/**
	 * Creates connections originating from the NIC with the given nicID to
	 * all other NICs.
	 * @param nicID ID of the NIC to create outgoing connections from
	 */
	void updateConnectionsFromNic(int nicId);

	node_t getAddr(NicEntry* nic);
	NicEntry* getNicFromAddress(node_t id);
};

#endif
