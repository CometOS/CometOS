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
 *  Created on: 19.04.2012
 */


#ifndef BASIC_ADDRESS_INTERFACE_H_
#define BASIC_ADDRESS_INTERFACE_H_

#include "MinAddressingBase.h"
#include "MacAbstractionBase.h"

namespace cometos {

/**
 * Simple abstract addressing base, providing getters and setters
 * to the addressing concepts supported by the MAC abstraction
 * layer: network id and node id.
 *
 * Uses the id parameter of each module to
 * store the short address, so that every module may
 * use the convenient getId() method to retrieve the
 * current ID.
 *
 * More sophisticated means of addressing should
 * extend this base class by inheriting.
 */
class MacAddressingBase : public MinAddressingBase {
public:
	MacAddressingBase(const char * service_name = NULL) :
		MinAddressingBase(service_name),
		nwkId(0)
	{}

	virtual void initialize() = 0;

	/**
	 * Get current short address of this node.
	 * @return current short address of this network node.
	 */
	virtual node_t getShortAddr() const = 0;

	/**
	 * Set the short address of this network node.
	 * @param  new short address
	 * @return true, if address was set successfully
	 * 		   false, if address could not be set to the given value
	 */
	virtual bool setShortAddr(node_t newAddr) = 0;

	/**
	 * Get the network (PAN) ID of this node.
	 * @return current network id of this node.
	 */
    virtual mac_networkId_t getNwkId() const;

    /**
     * Set the network ID of this node.
     * @param new network id
     * @return true, if network id was set successfully
     *         false, if network id could not be set to the given value
     */
    virtual bool setNwkId(mac_networkId_t id);


protected:
	mac_networkId_t nwkId;
};

inline mac_networkId_t MacAddressingBase::getNwkId() const{
	return nwkId;
}

inline bool MacAddressingBase::setNwkId(mac_networkId_t newId) {
	nwkId = newId;
	return true;
}

} // namespace

#endif /* ADDRESSINTERFACE_H_ */
