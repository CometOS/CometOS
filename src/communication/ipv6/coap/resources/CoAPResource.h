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


#ifndef COAPRESOURCE_H_
#define COAPRESOURCE_H_

#include "cometos.h"
#include "CoAPMessage.h"
#include "URL.h"
#include "Token.h"

namespace cometos_v6 {

class CoAPLayer;

const uint8_t COAP_RES_R =  0x01;   //READ
const uint8_t COAP_RES_W =  0x02;   //WRITE
const uint8_t COAP_RES_D =  0x04;   //DELETE
const uint8_t COAP_RES_H =  0x08;   //HIDDEN from resource discovery

enum {
    COAP_DUPLICATE_REQ_PIGGYBACK_SENT = 0,
    COAP_DUPLICATE_REQ_SEPERATE_SENT  = 1,
    COAP_DUPLICATE_REQ_NOTHING_SENT   = 2
};

#define RESOURCE_HANDLER_SIGNATURE      cometos_v6::CoAPMessage*
#define RESOURCE_HANDLER_PARAMETERS     cometos_v6::CoAPMessage* message
#define RESOURCE_HANDLER_CALLING        message

#define RESOURCE_COAP_HANDLER_SIGNATURE      cometos_v6::CoAPMessage*, const cometos_v6::URL&, uint16_t, uint8_t, uint8_t, uint16_t, cometos_v6::Token, cometos_v6::CoAPMessageOption**, uint8_t, const char*, uint16_t, bool, bool
#define RESOURCE_COAP_HANDLER_PARAMETERS
#define RESOURCE_COAP_HANDLER_CALLING

class CoAPResource {
public:
    CoAPResource():
        localPort(0),
        path(NULL),
        pathSize(0),
        options(COAP_RES_R),
        rt(NULL),
        coap(NULL),
        buffer(NULL) {}

    virtual ~CoAPResource(){
        if(this->rt != NULL) {
            delete[] this->rt;
            this->rt = NULL;
        }
        deletePath();
    }


    /*
     * This method sets the inbuild callback method to the specified method.
     * The specified method has to be a method of the CoAPLayer to send CoAPMessages!
     */
    void init(CoAPLayer* owner) {
        coap = owner;
    }

    uint8_t sendMessage(CoAPMessage* message);

    CoAPMessage* makeMessage();

    void initMessage(      cometos_v6::CoAPMessage* msg,
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
                           bool handlePayload);

    /*
     * This method sets the internal fields of the ressource.
     */
    void setRessourceData(const char** path, uint8_t pathSize, uint8_t options, uint16_t port, const char* rt);

    void setBuffer(CoAPBuffer_t* buf){
        buffer = buf;
    }

    virtual char* getRepresentation() = 0;
    virtual void setRepresentation(const char* repr, uint16_t len) = 0;

    char* getResourceType()
    {
        return rt;
    }

    /*
     * Takes an array of strings in the form of {"this","is","a","path","to","a","resource"}
     * and compares it to the path of this resource.
     */
    bool hasPath(char** path, uint8_t pathSize) {
        if(pathSize != this->pathSize)
            return false;

        for(uint8_t i = 0; i < pathSize; i++){
            if(strlen(path[i]) != strlen(this->path[i]))
                return false;
            if(strncmp(path[i], this->path[i], strlen(path[i])) != 0)
                return false;
        }
        return true;
    }

    /*
     * Takes a string in the form "/this/is/a/path/to/a/resource" and compares it to the
     * path of this resource.
     */
    bool hasPath(char* path) {
        uint16_t pos = 1;       // the first char is a '/' that we'll have to ignore

        for (uint8_t i = 0; i < pathSize; i++) {
            if (strncmp(&path[pos], this->path[i], strlen(this->path[i])) != 0) {
                return false;
            }
            pos += strlen(this->path[i]) + 1;       // add 1 to skip the next '/'
        }

        return true;
    }

    /*
     * Returns the length of the path.
     */
    uint16_t getPathLength(){
        uint16_t pathLength = 0;
        for(uint8_t i = 0; i < pathSize; i++)
            pathLength += strlen(path[i]);
        return pathLength + pathSize;
    }

    /*
     * Copies the resources path into the allocated buffer, if it fits.
     * It returns the number of bytes actually written.
     */
    uint16_t printPath(char* path, uint16_t maxLen);

    uint16_t getLocalPort() { return localPort; }

    /*
     * These functions will be queried according to what the request looks like.
     * If the request wants to DELETE this resource, but you don't allow any deletes on this resource
     * via the options, the request will be aborted and a matching response code will be generated.
     * If you do allow deletion, your resource has to deal with the DELETE command itself.
     * This includes checking for authorization!
     */
    bool isReadable()       { return options & COAP_RES_R; }
    bool isWritable()       { return options & COAP_RES_W; }
    bool isDeletable()      { return options & COAP_RES_D; }
    bool isHiddenFromResourceDiscovery() { return options & COAP_RES_H; }

    /*
     *  This function gets called by CoAPLayer if the request matches the URI.
     *  You should decide whether you want to return the response immediately or separately.
     *  You should call your matching handler for the request and return true if you want the CoAPLayer to send and ACK
     *  in case you plan on sending a separate response.
     *  If you return false, you will have to send the ACK or a piggyback message yourself.
     *
     *  You MUST send either an piggyback response or a separate response, the rest is handled by the CoAPLayer.
     *
     *  This function should check the method code and options to decide what it has to do.
     *  You are responsible for handling Options and deciding what options you accept.
     *  Attention: If you intend to not support an option, make sure to respond with a matching response code!
     */
    virtual void handleRequest(CoAPMessage* request) = 0;

    /*
     *  This function gets called by CoAPLayer if the request matches the URI,
     *  and the request was already processed.
     *
     *  There are three possible situations.
     *  Firstly, a piggyback response was lost -> type == COAP_DUPLICATE_REQ_PIGGYBACK_SENT
     *  Secondly, a seperate response was lost -> type == COAP_DUPLICATE_REQ_SEPERATE_SENT
     *  And thirdly, you didn't have sent anyting yet. No ACK, no piggyback and no separate response -> type == COAP_DUPLICATE_REQ_NOTHING_SENT
     *
     *  A lost ACK before sending a separate response is no problem, as the same ACK can be sent again.
     *
     *  You will have to decide based on the request and what the resource resembles, how you will handle the duplicate request.
     *  You MUST NOT rely on the sent response still being completly saved in the sentMessages array.
     *  Most likely only the metadata will be saved, or the metadata will be kept for a longer time.
     */
    virtual void handleDuplicateRequest(CoAPMessage* request, uint8_t type) = 0;

protected:

    void deletePath() {
        if(this->path != NULL){
            for(uint8_t i = 0; i < this->pathSize; i++){
                delete[] path[i];
                path[i] = NULL;
            }
            delete[] path;
            path = NULL;
        }
    }

    uint16_t localPort;
    char**  path;
    uint8_t pathSize;
    uint8_t options;

    char* rt;

    CoAPLayer* coap;

    CoAPBuffer_t* buffer;
};

}

#endif /* COAPRESOURCE_H_ */
