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


#include "IPv6InterfaceTable.h"
#include "palId.h"

#ifndef IP_PREFIX
#define IP_PREFIX   0x64,0x29,0x30,0x31,0x0,0x0,0x0
#endif

namespace cometos_v6 {

Define_Module(IPv6InterfaceTable);

IPv6InterfaceTable::IPv6InterfaceTable(const char * service_name) :
        cometos::Module(service_name),
        initialized(false)
{
}

void IPv6InterfaceTable::initialize() {
    cometos::Module::initialize();
    interfaces[0].MACAddress = Ieee802154MacAddress((uint16_t)palId_id());
    interfaces[0].addresses[0].setLinkLocal();
    interfaces[0].addresses[0].setAddressPart(palId_id(), 7);

#ifdef OMNETPP
    std::string prefix = par(("address"));
    interfaces[0].addresses[1].tryParse(prefix.c_str());
    if (par("isPrefix")) {
        interfaces[0].addresses[1].setAddressPart(palId_id(), 7);
    }
#else
    interfaces[0].addresses[1].set(IP_PREFIX,palId_id());
#endif

    initialized = true;
}

}
