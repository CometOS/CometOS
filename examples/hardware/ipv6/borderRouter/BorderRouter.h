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
 * @author Patrick Fuhlert
 */

#ifndef __COMETOS_V6_BORDERROUTER_H_
#define __COMETOS_V6_BORDERROUTER_H_

// standard
#include <sstream>
#include <iostream>
#include <iomanip>
#include "assert.h"

// cometos
#include "cometos.h"
#include "ICMPv6Message.h"
#include "Task.h"
#include "palLed.h"
#include "palId.h"
#include "IPv6Datagram.h"
#include "IPHCCompressor.h"
#include "IPHCDecompressor.h"
#include "LowpanBuffer.h"
#include "Ieee802154MacAddress.h"
#include "UDPLayer.h"
#include "UDPPacket.h"
#include "ICMPv6Message.h"
#include "ContentRequest.h"
#include "ContentResponse.h"
#include "SerialComm.h"
//#include "TcpComm.h"
#include "Dispatcher.h"
#include "Module.h"
#include "IpForward.h"
#include "LowpanAdaptionLayer.h"

// threading
#include "threadutils.h"
#include "palExec.h"
#include <time.h>

//tuntap
#include "TunTap.h"

// queue
#include <list>
#include <queue>

// xml parse
#include "include/pugixml/src/pugixml.hpp"

class BorderRouter : public cometos::Module
{
    public:

        BorderRouter(const char* ip_prefix);
        ~BorderRouter();
        void initialize();

        cometos_v6::IPv6Datagram* parseUDPIPv6(const uint8_t* buffer, uint16_t length);
        cometos_v6::IPv6Datagram* getInfo(IPv6Address reqIP, IPv6Address resIP, uint16_t dstPort);

        void fromExtern(cometos_v6::IPv6Datagram* dg);


        cometos::OutputGate<cometos_v6::IPv6Request> toLowpan;
        cometos::InputGate<cometos_v6::IPv6Request> fromLowpan;

        thread_mutex_t mutex;
        TunTap* tt;

        bool toIPv4;

    private:

//        void setInnerAddresses();
        bool isInnerAddress(IPv6Address addr);

        void handleResponseFromLowpan(cometos_v6::IPv6Response *response);
        void handleRequestFromLowpan(cometos_v6::IPv6Request *iprequest);

        void toExtern(cometos_v6::IPv6Datagram* dg);

        IPv6Address ip_prefix;
//        list<IPv6Address> innerAddresses;

};

#endif
