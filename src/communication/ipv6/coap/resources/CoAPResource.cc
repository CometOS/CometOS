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


#include "CoAPResource.h"
#include "CoAPLayer.h"

namespace cometos_v6 {

uint8_t CoAPResource::sendMessage(CoAPMessage* message){
    return coap->sendMessage(message, true);
}

CoAPMessage* CoAPResource::makeMessage(){
    return coap->prepareMessage();
}

void CoAPResource::initMessage(       cometos_v6::CoAPMessage* msg,
                                const cometos_v6::URL& url,
                                      uint16_t srcPort,
                                      uint8_t messageType,
                                      uint8_t messageCode,
                                      uint16_t messageID,
                                      cometos_v6::Token token,
                                      cometos_v6::CoAPMessageOption** options,
                                      uint8_t optionsLen,
                                const char* payload,
                                      uint16_t payloadLen,
                                      bool handleOptions,
                                      bool handlePayload)
{
    coap->initMessage(msg, url, srcPort,
                      messageType, messageCode,
                      messageID, token, options,
                      optionsLen, payload, payloadLen,
                      handleOptions, handlePayload);
}

uint16_t CoAPResource::printPath(char* path, uint16_t maxLen){
    uint16_t pos = 0;
    if(maxLen >= getPathLength())
        for(uint8_t i = 0; i < pathSize; pos += strlen(this->path[i]), i++){
            path[pos] = '/';
            memcpy(&path[++pos], this->path[i], strlen(this->path[i]));
        }
    return pos;
}

void CoAPResource::setRessourceData(const char** path, uint8_t pathSize, uint8_t options, uint16_t port, const char* rt)
{
    deletePath();

    this->path = new char*[pathSize];

    for(uint8_t i = 0; i < pathSize; i++) {
        this->path[i] = new char[strlen(path[i]) + 1];
        memcpy(this->path[i], path[i], strlen(path[i]) + 1);
    }
    this->pathSize = pathSize;
    this->options = options;
    this->localPort = port;

    if(rt != NULL) {
        if(this->rt != NULL) {
            delete[] this->rt;
        }
        this->rt = new char[strlen(rt)];
        memcpy(this->rt, rt, strlen(rt));
    }
}

}
