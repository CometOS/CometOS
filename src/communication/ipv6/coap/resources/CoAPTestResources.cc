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
 * @author: Marvin Burmeister
 */


#include "CoAPTestResources.h"
#include "URL.h"

 CoAPLightSensor::CoAPLightSensor(): lightIntensity(NULL) {}

 CoAPLightSensor::~CoAPLightSensor()
 {
      for(int i = 0; i < pathSize; i++){
          delete[] this->path[i];
          path[i] = NULL;
      }
      delete[] path;
      path = NULL;
      if(lightIntensity != NULL)
          delete[] lightIntensity;
 }

void CoAPLightSensor::handleRequest(cometos_v6::CoAPMessage* request)
{
    if(request == NULL)
        return;

    cometos_v6::CoAPMessage* answer = makeMessage();
    cometos_v6::URL url(cometos_v6::URL::COAP, request->getAddr(),
            request->getMessageSrcPort());

    uint8_t methodCode;

    if(request->getMessageCode() == cometos_v6::COAP_GET)
    {      //If this request is a GET request
        methodCode = cometos_v6::COAP_CONTENT;
    }
    else
    {      //If a method is used, that is not supported
        methodCode = cometos_v6::COAP_METHODNOTALLOWED;
    }
    initMessage(answer,
            url,
            localPort,
            cometos_v6::COAP_ACK,
            methodCode,
            request->getMessageID(),
            request->getMessageToken(),
            NULL, 0,
            lightIntensity, strlen(lightIntensity),
            false, false);


    sendMessage(answer);
 }

void CoAPLightSensor::handleDuplicateRequest(cometos_v6::CoAPMessage* request,
         uint8_t type){}


char* CoAPLightSensor::getRepresentation(){
    return lightIntensity;
}
void CoAPLightSensor::setRepresentation(const char* repr, uint16_t len){
    if(lightIntensity != NULL) {
        delete[] lightIntensity;
    }
    lightIntensity = new char[len + 1];
    memcpy(lightIntensity, repr, len + 1);
}


char* CoAPLightSwitch::getRepresentation(){
    return switchState;
}
void CoAPLightSwitch::setRepresentation(const char* repr, uint16_t len){
    if(switchState != NULL) {
        delete[] switchState;
    }
    switchState = new char[len + 1];
    memcpy(switchState, repr, len + 1);
}

 CoAPLightSwitch::CoAPLightSwitch(): switchState(NULL) {}

 CoAPLightSwitch::~CoAPLightSwitch()
 {
      for(int i = 0; i < pathSize; i++){
          delete[] this->path[i];
          path[i] = NULL;
      }
      delete[] path;
      path = NULL;

      if(switchState != NULL) {
          delete[] switchState;
      }

 }

 void CoAPLightSwitch::handleRequest(cometos_v6::CoAPMessage* request)
 {
    if(request == NULL) {
        return;
    }

    uint8_t methodCode;

    cometos_v6::CoAPMessage* answer = makeMessage();

    cometos_v6::URL url(cometos_v6::URL::COAP, request->getAddr(),
            request->getMessageSrcPort());

     if(request->getMessageCode() == cometos_v6::COAP_GET)
     {      //If this request is a GET request
         methodCode = cometos_v6::COAP_CONTENT;
     }
     else if(request->getMessageCode() == cometos_v6::COAP_POST &&
             request->getMessagePayloadLen())
     {   //If this request is a POST request and contains a representation in the payload
         setRepresentation((char*) request->getMessagePayload(),
                 request->getMessagePayloadLen());

         methodCode = cometos_v6::COAP_CHANGED;
     }
     else if(request->getMessageCode() == cometos_v6::COAP_POST)
     {   //If this request is a POST request but doesn't contain a representation
         methodCode = cometos_v6::COAP_NOTIMPLEMENTED;
     }
     else
     {   //If a method is used, that is not supported
         methodCode = cometos_v6::COAP_METHODNOTALLOWED;
     }
     initMessage(answer,
             url,
             localPort,
             cometos_v6::COAP_ACK,
             methodCode,
             request->getMessageID(),
             request->getMessageToken(),
             NULL, 0,
             switchState, strlen(switchState),
             false, false);
     sendMessage(answer);
 }

 void CoAPLightSwitch::handleDuplicateRequest(cometos_v6::CoAPMessage* request,
         uint8_t type)
 {

 }
