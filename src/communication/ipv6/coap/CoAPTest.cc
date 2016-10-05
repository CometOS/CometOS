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
 * @author: Marvin Burmeister, Martin Ringwelski
 */

#include "CoAPTest.h"
#include "CoAPTestResources.h"
#include "palId.h"
#include "NeighborDiscovery.h"

Define_Module(CoAPTest);

uint16_t dstAddressTemplate[8] = {
        IP_NWK_PREFIX
};

CoAPTest::CoAPTest(const char * service_name):
        cometos::LongScheduleModule(service_name),
        coapLayer(NULL),
        receiver(0),
        sender(false),
        port(cometos_v6::COAP_PORT),
        counter(0),
        counter2(0),
        lightOn(false),
        resPaths(NULL),
        resTypes(NULL),
        numRes(0),
        serverResources{NULL, NULL}
{}

void CoAPTest::sendTimerFired(cometos::LongScheduleMsg * msg){

    if(palId_id() == receiver)
    {
        if (counter == 0)
        {
            const char* uri0[2] = {"sensors", "light0"};
            const char* uri1[2] = {"switches", "light0"};
            serverResources[0] = new CoAPLightSensor();
            serverResources[1] = new CoAPLightSwitch();

            serverResources[0]->setRessourceData(
                    uri0, 2, 0x00, this->port, "lightSensor");
            serverResources[1]->setRessourceData(
                    uri1, 2, 0x00, this->port, "lightSwitch");

            serverResources[1]->setRepresentation("off", 3);
            serverResources[0]->setRepresentation("0 Lumen", 7);

            coapLayer->insertResource(serverResources[0]);
            coapLayer->insertResource(serverResources[1]);

            counter++;
            longSchedule(msg, (cometos::LongScheduleModule::longTimeOffset_t) 500);
        }
        else
        {
            if(strncmp(serverResources[1]->getRepresentation(), "on", 2) == 0)
                serverResources[0]->setRepresentation("100 Lumen", 9);
            else if(strncmp(serverResources[1]->getRepresentation(), "off", 3) == 0)
                serverResources[0]->setRepresentation("0 Lumen", 7);
            else
            {
                serverResources[1]->setRepresentation("off", 3);
                serverResources[0]->setRepresentation("0 Lumen", 7);
            }

            longSchedule(msg, (cometos::LongScheduleModule::longTimeOffset_t) 5000);
        }
    }
    if(sender)
    {
        bool handleOptions = false;
        bool handlePayload = false;
        //TODO delete resPaths / resTypes upon finish
        int8_t error = 0;
        if(numRes == 0) { //If no resources were found yet, discover them
            LOG_DEBUG("Send Resource Discovery");
            cometos_v6::URL url(cometos_v6::URL::COAP, IPv6Address(IP_NWK_PREFIX));
            url.parseURIPart(cometos_v6::CoAPLayer::RESOURCE_DISCOVERY_PATH);
            error = coapLayer->sendMessage(
                    url,
                    port,
                    cometos_v6::COAP_CON,
                    cometos_v6::COAP_GET,
                    NULL, 0,
                    NULL, 0,
                    handleOptions,
                    handlePayload,
                    0); //Ressource Discovery
        } else {   //we found resources, GET a representation of all of them
            cometos_v6::URL url(cometos_v6::URL::COAP, IPv6Address(IP_NWK_PREFIX));

            if(counter < numRes) {
                LOG_DEBUG("Get State of Resources");
                url.parseURIPart(resPaths[(counter) % numRes]);
                error = coapLayer->sendMessage(
                        url,
                        port,
                        cometos_v6::COAP_CON,
                        cometos_v6::COAP_GET,
                        NULL, 0,
                        NULL, 0,
                        handleOptions,
                        handlePayload,
                        1);
            }
            else if(counter < numRes * 2){
                url.parseURIPart(resPaths[counter % numRes]);
                if(strncmp(resTypes[counter % numRes], "lightSwi", 11) == 0)
                {
                    if(lightOn) {
                        LOG_DEBUG("Send Request to turn Light off");
                        error = coapLayer->sendMessage(
                                url,
                                port,
                                cometos_v6::COAP_CON,
                                cometos_v6::COAP_POST,
                                NULL, 0,
                                "off", 3,
                                handleOptions,
                                handlePayload,
                                2);
                    } else {
                        LOG_DEBUG("Send Request to turn Light on");
                        error = coapLayer->sendMessage(
                                url,
                                port,
                                cometos_v6::COAP_CON,
                                cometos_v6::COAP_POST,
                                NULL, 0,
                                "on", 2,
                                handleOptions,
                                handlePayload,
                                2);
                    }
                }
                else if(strncmp(resTypes[counter % numRes], "lightSen", 11) == 0)
                {
                    if(lightOn) {
                        LOG_DEBUG("Send Request to change lightSensor to 150");
                        error = coapLayer->sendMessage(
                                url,
                                port,
                                cometos_v6::COAP_CON,
                                cometos_v6::COAP_POST,
                                NULL, 0,
                                "150", 3,
                                handleOptions,
                                handlePayload,
                                2);
                    } else {
                        LOG_DEBUG("Send Request to change lightSensor to 10");
                        error = coapLayer->sendMessage(
                                url,
                                port,
                                cometos_v6::COAP_CON,
                                cometos_v6::COAP_POST,
                                NULL, 0,
                                "10", 2,
                                handleOptions,
                                handlePayload,
                                2);
                    }
                }

            }
            counter++;
        }

        if (error) {
            LOG_ERROR("Error: " << (int)error);
        }

        if (numRes > 0 && counter == (2*numRes)) {
            counter = 0;
            counter2++;
        }
        if (counter2 == 5) {
            delete msg;
            return;
        }
        longSchedule(msg, (cometos::LongScheduleModule::longTimeOffset_t) 1500);
    }
}

void CoAPTest::incommingCoAPHandler(const cometos_v6::CoAPMessage* incoming, const bool state){
    uint8_t tag = incoming->getMessageTag();
    if(state)
    {
        if(tag == 0)        //Answer to resource discovery
        {
            LOG_DEBUG("CoAPTest rcvd ans to ResDisc. Port: " << port);
        }
        else if(tag == 1)   //Answer to a GET command
        {
            LOG_DEBUG("CoAPTest rcvd ans of GET req. Port: " << port);
        }
        else if(tag == 2)   //Answer to a POST command
        {
            LOG_DEBUG("CoAPTest rcvd ans of POST req. Port: " << port);
        }

        if(incoming->getMessagePayloadLen())
        {
            char* payload = new char[incoming->getMessagePayloadLen() + 1];
            memcpy(payload, incoming->getMessagePayload(), incoming->getMessagePayloadLen());
            payload[incoming->getMessagePayloadLen()] = 0;
            LOG_DEBUG("Included representation: " << payload);
            //if this is answer to a resource discovery, then parse the found resources
            if(tag == 0)
            {
                numRes = strlen(payload) ? 1 : 0;

                for(uint16_t i = 0; i < strlen(payload); i++){
                    if(payload[i] == ',')
                        numRes++;
                }

                resPaths = new char*[numRes];
                resTypes = new char*[numRes];

                uint16_t uriBegin = 0;
                uint16_t uriLen = 0;
                uint8_t uriCounter = 0;
                for(uint16_t i = 0; i < strlen(payload); i++){
                    if(payload[i] == '<')
                        uriBegin = i + 1;
                    else if(payload[i] == '>'){
                        uriLen = i - uriBegin + 1;
                        resPaths[uriCounter] = new char[uriLen];
                        memcpy(resPaths[uriCounter], &payload[uriBegin], uriLen);
                        resPaths[uriCounter][uriLen - 1] = '\0';
                    }
                    else if(strncmp(&payload[i], ";rt=\"", 5) == 0){
                        i += 5;
                        uriBegin = i;
                    }
                    else if(payload[i] == '"'){
                        uriLen = i - uriBegin + 1;
                        resTypes[uriCounter] = new char[uriLen];
                        memcpy(resTypes[uriCounter], &payload[uriBegin], uriLen);
                        resTypes[uriCounter][uriLen - 1] = '\0';
                        uriCounter++;
                    }

                }
            }
            else if(tag == 2)
            {
                if(strncmp(payload, "on", 2) == 0)
                    lightOn = true;
                else if(strncmp(payload, "off", 2) == 0)
                    lightOn = false;
            }
            delete[] payload;
        }

    }
    else {
        LOG_DEBUG("CoAPTest received no answer! Port: " << port);
    }
}

void CoAPTest::initialize() {
    coapLayer = (cometos_v6::CoAPLayer*) getModule(cometos_v6::CoAPLayer::MODULE_NAME);
    coapLayer->bindPort(this, cometos_v6::COAP_PORT);

    CONFIG_NED(sender);

    CONFIG_NED(receiver);

    if(sender) {
       LOG_DEBUG("sender");
       longSchedule(new cometos::LongScheduleMsg(
                   createCallback(&CoAPTest::sendTimerFired)),
               (cometos::LongScheduleModule::longTimeOffset_t) 10000);
    }
    if(palId_id() == receiver) {
        LOG_DEBUG("receiver");
        longSchedule(new cometos::LongScheduleMsg(
                    createCallback(&CoAPTest::sendTimerFired)),
                (cometos::LongScheduleModule::longTimeOffset_t) 1000);
    }

}

void CoAPTest::finish(){

}
