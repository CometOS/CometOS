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
 * @author: Martin Ringwelski
 */


#ifndef IPV6INTERFACETABLE_H_
#define IPV6INTERFACETABLE_H_

#include "cometos.h"

#include "IPv6Address.h"
#include "Ieee802154MacAddress.h"

namespace cometos_v6 {

class IPv6InterfaceTable;


class IPv6Interface {
    friend IPv6InterfaceTable;
public:
    IPv6Interface()
    {};

    const IPv6Address& getAddress(uint8_t num) const {
        if (num < getNumAddresses()) {
            return addresses[num];
        } else {
            return addresses[0];
        }
    }

    uint8_t getNumAddresses() const {
        return 2;
    }

    const IPv6Address& getLocalAddress() const {
        return addresses[0];
    }
    const IPv6Address& getGlobalAddress() const {
        return addresses[1];
    }
    void setLocalAddress(IPv6Address& addr) {
        addresses[0] = addr;
    }
    void setGlobalAddress(IPv6Address& addr) {
        addresses[1] = addr;
    }

    const Ieee802154MacAddress & getMacAddress() {
        return MACAddress;
    }

private:
    IPv6Address addresses[2];
    Ieee802154MacAddress MACAddress;
};

#define INTERFACE_TABLE_MODULE_NAME "it"

class IPv6InterfaceTable: public cometos::Module {
public:
    IPv6InterfaceTable(const char * service_name = NULL);
    virtual ~IPv6InterfaceTable() {}
    void initialize();

    uint8_t getNumInterfaces() const {
        return 1;
    }

    IPv6Interface& getInterface(uint8_t iNum) {
        if (!initialized) {
            initialize();
        }
        if (iNum < this->getNumInterfaces()) {
            return (interfaces[iNum]);
        }
        return (interfaces[0]);
    }

protected:
    IPv6Interface interfaces[1];
private:
    bool initialized;
};

}

#endif /* IPV6INTERFACETABLE_H_ */
