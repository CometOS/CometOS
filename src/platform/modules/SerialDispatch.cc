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

#include "SerialDispatch.h"
#include "mac_definitions.h"
/*MACROS---------------------------------------------------------------------*/

// not used in Omnet++, thus no Define_Module

namespace cometos {

uint16_t SerialDispatch::portNumber = 0;


SerialOutput::SerialOutput(const char* port,
                           uint16_t id,
                           SerialDispatch* ptr,
                           uint32_t baudrate,
                           uint16_t frameTimeout) :
                                   gateOut(nullptr),
                                   gateIn(nullptr),
                                   sc(nullptr)
{
    // check if the created module name fits the restricted length
    // e.g. for a MODULE_NAME_LENGTH of 5, we have three hex digits remaining
    // for the id, meaning the id has to be lower than 4096
    uint16_t numDigitsShiftValue = 4 * (MODULE_NAME_LENGTH - 2);
    bool idTooManyDigits = (id >> numDigitsShiftValue) > 0;
    if (idTooManyDigits) {
        LOG_ERROR("Module name length not suppoprted");
        ASSERT(!idTooManyDigits);
        return ;
    }
    char name[MODULE_NAME_LENGTH];
    sprintf(name, "s%03u", id);

    char gatenameIn[MODULE_NAME_LENGTH+1];
    char gatenameOut[MODULE_NAME_LENGTH+1];
    sprintf(gatenameIn, "gi%03x", id);
    sprintf(gatenameOut, "go%03x", id);

    sc = new SerialComm(port, name, baudrate, frameTimeout);
    sc->initialize();

    gateIn = new InputGate<DataIndication>(ptr, &SerialDispatch::handleIndication, gatenameIn);
    gateOut = new OutputGate<DataRequest>(ptr, gatenameOut);

    // connect gates
    gateOut->connectTo(sc->gateReqIn);
    sc->gateIndOut.connectTo(*gateIn);

    getCout() << "created/connected SerialComm (" << id << ") to " << port
              << " and connected gates;"
              << " objptr=" << (uintptr_t) this
              << " modname=" << name << cometos::endl;
}

SerialOutput::~SerialOutput() {
    delete gateIn;
    delete gateOut;
    delete sc;
    gateIn = nullptr;
    gateOut = nullptr;
    sc = nullptr;
}


void SerialOutput::sendRequest(DataRequest* msg) {
    gateOut->send(msg);
}

bool SerialOutput::initializationSuccessful() {
    return sc != nullptr
            && gateOut != nullptr
            && gateIn != nullptr;

}


const char* const SerialDispatch::DEFAULT_MODULE_NAME = "serd";

SerialDispatch::SerialDispatch() :
        cometos::LowerEndpoint(DEFAULT_MODULE_NAME)
{}

SerialDispatch::~SerialDispatch() {
    for (auto it : forwarderMap) {
        getCout() << "calling serialFwd dtor for node "
                  << it.first << " on ptr=" << (uintptr_t) it.second << cometos::endl;
        delete it.second;
        it.second = nullptr;
    }
}

void SerialDispatch::initialize() {
    LowerEndpoint::initialize();
    schedule(new cometos::Message(), &SerialDispatch::initializeGates, 0);
}

void SerialDispatch::initializeGates(cometos::Message* msg) {
    delete msg;
    msg = nullptr;

    // call subclass
    doInitializeGates();
}

void SerialDispatch::handleIndication(cometos::DataIndication* msg) {
    // just forward to upper module
    sendIndication(msg);
}


void SerialDispatch::handleRequest(cometos::DataRequest* msg) {
    if (msg->dst == MAC_BROADCAST) {
        if (forwarderMap.size() == 1) {
            LOG_DEBUG("Forward broadcast to only connected serial");
            forwarderMap.begin()->second->sendRequest(msg);
        } else {
            LOG_WARN("NO broadcasts over SerialDispatch allowed until there are valid copy constructors for DataRequest etc.");
            msg->response(new DataResponse(false));
            delete(msg);
            return;
        }
    } else {
        auto it = forwarderMap.find(msg->dst);
        if (it == forwarderMap.end()) {
            LOG_DEBUG("discard request for " << msg->dst << "; no gate found");
            ASSERT(it != forwarderMap.end());
            msg->response(new DataResponse(false));
            delete msg;
            return;
        } else {
            LOG_DEBUG("forward request for " << msg->dst);
            SerialOutput* out = forwarderMap.at(msg->dst);
            ASSERT(out != NULL);
            out->sendRequest(msg);
        }
    }
}


cometos_error_t SerialDispatch::createForwarding(
                const char* port,
                node_t address,
                uint32_t baudrate,
                uint16_t frameTimeout)
{
    // we expect that all initialization has been carried out
    // when this method is called --- otherwise there may be double
    // initializations (due to the initialize calls from the module list)
    auto it = forwarderMap.find(address);
    if (it != forwarderMap.end()) {
        return COMETOS_ERROR_ALREADY;
    } else {
        // create new SerialComm, OutputGate, initialize it, and connect gates
        SerialOutput* so = new SerialOutput(port, portNumber, this, baudrate, frameTimeout);
        portNumber++;
        if (so->initializationSuccessful()) {
            forwarderMap[address] = so;
            return COMETOS_SUCCESS;
        } else {
            delete so;
            return COMETOS_ERROR_FAIL;
        }
    }
}

}
