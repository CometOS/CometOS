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
 * @author Martin Ringwelski
 */

#include "Bounce.h"
#include "logging.h"

Define_Module(Bounce);

using namespace cometos;

Bounce::Bounce(const char * service_name) :
        cometos::Module(service_name),
        MACinput(this, &Bounce::handleMACRequest, "MACinput"),
        MACoutput(this, "MACoutput")
{}

void Bounce::initialize() {

}

void Bounce::handleMACRequest(DataRequest *macrequest) {
    if (macrequest->dst == 0xFFFF) {
        LOG_DEBUG("Recieved DataRequest, Length " << (int)macrequest->getAirframe().getLength() << " First Symbols:");
    } else {
        LOG_DEBUG("Recieved Forwarded DataRequest, Length " << (int)macrequest->getAirframe().getLength() << " First Symbols:");
    }
    LOG_DEBUG(std::hex << (int)macrequest->getAirframe().getData()[0]
               << " " << std::hex << (int)macrequest->getAirframe().getData()[1]
               << " " << std::hex << (int)macrequest->getAirframe().getData()[2]
               << " " << std::hex << (int)macrequest->getAirframe().getData()[3]
               << " " << std::hex << (int)macrequest->getAirframe().getData()[4]
               << " " << std::hex << (int)macrequest->getAirframe().getData()[5]
               << " " << std::hex << (int)macrequest->getAirframe().getData()[6]
               << " " << std::hex << (int)macrequest->getAirframe().getData()[7]
               << " " << std::hex << (int)macrequest->getAirframe().getData()[8]
               << " " << std::hex << (int)macrequest->getAirframe().getData()[9]
               << " " << std::hex << (int)macrequest->getAirframe().getData()[10]
               << " " << std::hex << (int)macrequest->getAirframe().getData()[11]
               << " " << std::hex << (int)macrequest->getAirframe().getData()[12]
               << " " << std::hex << (int)macrequest->getAirframe().getData()[13]
               << " " << std::hex << (int)macrequest->getAirframe().getData()[14]
               << " " << std::hex << (int)macrequest->getAirframe().getData()[15]);
    LOG_DEBUG(std::hex << (int)macrequest->getAirframe().getData()[16]
               << " " << std::hex << (int)macrequest->getAirframe().getData()[17]
               << " " << std::hex << (int)macrequest->getAirframe().getData()[18]
               << " " << std::hex << (int)macrequest->getAirframe().getData()[19]
               << " " << std::hex << (int)macrequest->getAirframe().getData()[20]
               << " " << std::hex << (int)macrequest->getAirframe().getData()[21]
               << " " << std::hex << (int)macrequest->getAirframe().getData()[22]
               << " " << std::hex << (int)macrequest->getAirframe().getData()[23]
               << " " << std::hex << (int)macrequest->getAirframe().getData()[24]
               << " " << std::hex << (int)macrequest->getAirframe().getData()[25]
               << " " << std::hex << (int)macrequest->getAirframe().getData()[26]
               << " " << std::hex << (int)macrequest->getAirframe().getData()[27]
               << " " << std::hex << (int)macrequest->getAirframe().getData()[28]
               << " " << std::hex << (int)macrequest->getAirframe().getData()[29]
               << " " << std::hex << (int)macrequest->getAirframe().getData()[30]
               << " " << std::hex << (int)macrequest->getAirframe().getData()[31]);
    macrequest->response(new DataResponse(DataResponseStatus::SUCCESS));
//    for (uint8_t i = 0; i < macrequest->getAirframe().getLength(); i++) {
//        printf("%02X ", macrequest->getAirframe().getData()[i]);
//        if (i%16 == 15) printf("\n");
//    }

    if (macrequest->dst == 0xFFFF) {
        cometos::AirframePtr frame = macrequest->decapsulateAirframe();
        cometos::DataIndication * ind =  new cometos::DataIndication(frame, macrequest->dst, getId());
        delete(macrequest);

        MACoutput.send(ind);
    } else {


        delete(macrequest);
    }

}
