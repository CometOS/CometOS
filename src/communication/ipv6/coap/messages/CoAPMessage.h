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
 *  @author: Marvin Burmeister, Martin Ringwelski
 */

#ifndef COAPMESSAGE_H
#define COAPMESSAGE_H

#include "cometos.h"
#include "UDPLayer.h"
#include "LongScheduleModule.h"
#include "LowpanAdaptionLayer.h"
#include "CoAPMessageOption.h"
#include "URL.h"
#include "Token.h"
#include "SString.h"
#include <math.h>

#include "coapconfig.h"

namespace cometos_v6 {

const uint16_t COAP_PORT = 5683;

const uint8_t COAP_M_OPT_IFMATCH_LEN_MIN =        0;
const uint8_t COAP_M_OPT_IFMATCH_LEN_MAX =        8;

const uint8_t COAP_M_OPT_ETAG_LEN_MIN =           1;
const uint8_t COAP_M_OPT_ETAG_LEN_MAX =           8;

const uint8_t COAP_M_OPT_PATH_LEN_MIN =           0;
const uint8_t COAP_M_OPT_PATH_LEN_MAX =           255;

const uint8_t COAP_M_OPT_QUERY_LEN_MIN =          0;
const uint8_t COAP_M_OPT_QUERY_LEN_MAX =          255;

const uint8_t COAP_M_OPT_HOST_LEN_MIN =           1;
const uint8_t COAP_M_OPT_HOST_LEN_MAX =           255;

const uint16_t COAP_M_OPT_PROXYURI_LEN_MIN =      1;
const uint16_t COAP_M_OPT_PROXYURI_LEN_MAX =      1034;

const uint8_t COAP_M_OPT_PROXYURISCHEME_LEN_MIN = 1;
const uint8_t COAP_M_OPT_PROXYURISCHEME_LEN_MAX = 255;

//METHODCODES
enum {
    COAP_EMPTY =                    0x00,

    COAP_GET =                      0x01,
    COAP_POST =                     0x02,
    COAP_PUT =                      0x03,
    COAP_DELETE =                   0x04,

    COAP_CREATED =                  0x41,
    COAP_DELETED =                  0x42,
    COAP_VALID =                    0x43,
    COAP_CHANGED =                  0x44,
    COAP_CONTENT =                  0x45,

    COAP_BAD_REQUEST =              0x80,
    COAP_UNAUTHORIZED =             0x81,
    COAP_BADOPTION =                0x82,
    COAP_FORBIDDEN =                0x83,
    COAP_NOTFOUND =                 0x84,
    COAP_METHODNOTALLOWED =         0x85,
    COAP_NOTACCEPTABLE =            0x86,
    COAP_PRECONDITIONFAILED =       0x8C,
    COAP_REQUESTENTITYTOOLARGE =    0x8D,
    COAP_UNSUPPORTEDCONTENTFORMAT = 0x8F,

    COAP_INTERNALSERVERERROR =      0xA0,
    COAP_NOTIMPLEMENTED =           0xA1,
    COAP_BADGATEWAY =               0xA2,
    COAP_SERVICEUNAVAILABE =        0xA3,
    COAP_GATEWAYTIMEOUT =           0xA4,
    COAP_PROXYINGNOTSUPPORTED =     0xA5
};

//CONTENT FORMAT REGISITRY
enum {
    COAP_CF_TEXTPLAIN =     0,
    COAP_CF_LINKFORMAT =   40,
    COAP_CF_XML =          41,
    COAP_CF_OCTETSTREAM =  42,
    COAP_CF_EXI =          47,
    COAP_CF_JSON =         50
};

enum CoapType_t {
    COAP_CON = 0,
    COAP_NON = 1,
    COAP_ACK = 2,
    COAP_RST = 3
};

const uint8_t COAP_VERSION = 1;

//See draft-ietf-core-coap-18 4.8 - page 27
#define COAP_RANDOM_FACTOR      1.5
const uint8_t COAP_ACK_TIMEOUT =        2;       //2s
const uint8_t COAP_MAX_RETRANSMIT =     4;
const uint8_t COAP_NSTART =             1;
const uint8_t COAP_DEFAULT_LEISURE =    5;       //5s
const uint8_t COAP_PROBING_RATE =       1;       //1 Byte/s

//See draft-ietf-core-coap-18 4.8.2 - page 28-30
const uint16_t COAP_MAX_TRANSMIT_SPAN = COAP_ACK_TIMEOUT * (pow(2, COAP_MAX_RETRANSMIT) - 1) * COAP_RANDOM_FACTOR;
const uint16_t TRANSMIT_WAIT =          COAP_ACK_TIMEOUT * (pow(2, COAP_MAX_RETRANSMIT + 1) - 1) * COAP_RANDOM_FACTOR;
const uint16_t COAP_MAX_LATENCY =       100;
const uint8_t COAP_PROCESSING_DELAY =   COAP_ACK_TIMEOUT;
const uint16_t COAP_MAX_RTT =           2 * COAP_MAX_LATENCY + COAP_PROCESSING_DELAY;
const uint16_t COAP_EXCHANGE_LIFETIME = COAP_MAX_TRANSMIT_SPAN + (2 * COAP_MAX_LATENCY) + COAP_PROCESSING_DELAY;
const uint16_t COAP_NON_LIFETIME =      COAP_MAX_TRANSMIT_SPAN + COAP_MAX_LATENCY;

enum {
    COAP_MESSAGE_OK =                   0x00,
    COAP_MESSAGE_FORMATERROR =          0x01,
    COAP_MESSAGE_SILENTIGNORE =         0x02,
    COAP_MESSAGE_UNREC_OPTION =         0x04,
    COAP_MESSAGE_URL_UNREC_PREFIX =     0x08,
    COAP_MESSAGE_URL_UNREC_ADDRESS =    0x10,
    COAP_MESSAGE_URL_UNREC_PATH =       0x20,
    COAP_MESSAGE_ILLEGAL_TYPE_OR_CODE = 0x40,
    COAP_MESSAGE_QUEUE_FULL =           0x80,
    COAP_MESSAGE_MAX_ERROR =            0xFF
};

class CoAPMessage {
    friend class CoAPLayer;
    template<uint8_t N>friend class SentMessages;
public:
    //Definition of all options with their OptionNr
    enum ErrorCodes_t : int8_t {
        OPT_GO_ON =                  1,
        OPT_SUCC =                   0,
        OPT_UNREC_OPTION =           -1,
        OPT_MESSAGE_FORMAT_ERROR =   -2,
        OPT_UNKNOWN_PROTOCOL =       -3,
        OPT_UNKNOWN_ERROR =          -4
    };

    /*ErrorCodes_t init(const URL& url, uint16_t srcPort, uint8_t messageType,
            uint8_t messageCode, CoAPMessageOption** options,
            uint8_t optionsLen, uint8_t* payload, uint16_t payloadLen,
            bool handleOptions, bool handlePayload);*/
    int8_t init(const URL& url, uint16_t srcPort, uint8_t messageType,
            uint8_t messageCode, uint16_t messageID, Token token,
            CoAPMessageOption** options, uint8_t optionsLen, const char* payload,
            uint16_t payloadLen, bool handleOptions, bool handlePayload);

    int8_t setURL(const URL& url, CoAPMessageOption** options,
            uint8_t optionsMaxLen);

    cometos::SString<255> getMessageString();

    const IPv6Address& getAddr() const { return addr; }

    CoAPMessage(CoAPBuffer_t* buffer);

    virtual ~CoAPMessage();

    uint8_t getMessageVersion() const {
        return (biPacket->getContent()[0]>>6)&0x3;
    }
    CoapType_t getMessageType() const {
        return (CoapType_t)((biPacket->getContent()[0]>>4)&0x3);
    }
    uint8_t getMessageTokenLen() const {
        return biPacket->getContent()[0]&0xF;
    }
    uint8_t getMessageCode() const {
        return biPacket->getContent()[1];
    }
    uint16_t getMessageID() const {
        return (biPacket->getContent()[2]<<8) + biPacket->getContent()[3];
    }
    uint8_t getMessageTimer() const {
        return metaData.timer;
    }
    uint8_t getMessageTag() const {
        return metaData.tag;
    }

    void setMessageAddress(const IPv6Address& addr) {
        this->addr = addr;
    }

    Token getMessageToken () const {
        Token token;
        token.readToken((biPacket->getContent() + 4), getMessageTokenLen());

        return token;
    }

    uint16_t getMessageSrcPort() const {
        return metaData.srcPort;
    }
    uint16_t getMessageDstPort() const {
        return metaData.dstPort;
    }
    uint8_t getMessageRetries() const {
        return metaData.retries;
    }
    uint16_t getMessagePacketLen() const {
        return biPacket->getSize();
    }
    uint16_t getMessageOptionsLen() const {
        return metaData.optionsLength;
    }
    uint16_t getMessagePayloadLen() const {
        return metaData.payloadLength;
    }
    uint8_t getMessageError() const {
        return metaData.error;
    }

    const uint8_t* getMessageOptions() const {
        // Header - Token
        return &biPacket->getContent()[4 + getMessageTokenLen()];
    }
    const uint8_t* getMessagePayload() const {
        if(getMessagePayloadLen()) {
            // Header - Token - Options - PayloadIndicator
            return &biPacket->getContent()[4 + getMessageTokenLen() + getMessageOptionsLen() + 1];
        }
        return NULL;
    }

    const uint8_t* getMessagePacket() const {
        if(biPacket == NULL) {
            return NULL;
        }
        return biPacket->getContent();
    }

    bool isRequest() {
        codeMeaning_t req;
        getMessageCodeMeaning(req);
        return (req == REQUEST);
    }
    bool isResponse() {
        codeMeaning_t res;
        getMessageCodeMeaning(res);
        return (res == RESPONSE);
    }
    uint16_t getPathString(char*& path) {
        uint16_t pathLen = 0;
        CoAPMessageOption* pathOpts[COAP_MAX_MESSAGE_OPTIONS];
        uint8_t numOptions = findInCoAPMessageOptions(
                CoAPMessageOption::OPT_URIPATH, pathOpts, COAP_MAX_MESSAGE_OPTIONS);

        for(uint8_t i = 0; i < numOptions; i++) {
            pathLen += pathOpts[i]->getOptionLen() + 1;     //+1 for the '/' 's
        }

        path = new char[pathLen + 1];                             //+1 for the '\0'
        path[pathLen] = 0;
        uint16_t skip = 0;
        for(uint8_t i = 0; i < numOptions; i++){
            path[skip] = '/';
            skip++;
            memcpy(&path[skip],
                    pathOpts[i]->getOptionData(),
                    pathOpts[i]->getOptionLen());
            skip += pathOpts[i]->getOptionLen();

            delete pathOpts[i];
        }
        return pathLen;
    }

    void onlyKeepMetaData() {
        uint16_t headerSize = 4 + getMessageTokenLen();
        if (biPacket->getSize() > headerSize) {
            biPacket->freeEnd(biPacket->getSize() - headerSize);
        }
    }

private:
    // Use this Constructor to make a new Encoded Packet if you have a the
    // encoded data already
    CoAPMessage(CoAPBuffer_t* buffer, const IPv6Address& addr,
            uint16_t srcPort, uint16_t dstPort, const uint8_t* packet,
            uint16_t packetLen);

//    void deleteMessageBody() {
//        if (biPacket != NULL) {
//            biPacket->free();
//            biPacket = NULL;
//        }
//    }

    enum codeMeaning_t : uint8_t {
        ERROR, EMPTY, REQUEST, RESPONSE
    };
    int getMessageCodeMeaning(codeMeaning_t& meaning);

    void produceMessageFormatError(){
        uint8_t tokenLen = getMessageTokenLen();
        uint8_t invalidData = 0xF0;
        biPacket->copyToBuffer(&invalidData, 1, 4 + tokenLen);
    }

    cometos::SString<100> getErrorCodeDescriptions(uint8_t error);

    bool isEmpty(){
        codeMeaning_t empty;
        getMessageCodeMeaning(empty);
        return (empty == EMPTY);
    }
    int findInCoAPMessageOptions(uint16_t optionNr, CoAPMessageOption** opts,
            uint8_t optsSize);
    int decodeCoAPMessageOptions(CoAPMessageOption** opts, uint8_t optsSize);

    uint8_t calculateTimer(uint8_t retries){
        // generate a random number between ACK_TIMEOUT and
        // ACK_RANDOM_FACTOR (X*1000/1000 is to overcome that modulo doesn't
        // work with double/float)
        // for standard values it generates an timout of either 2 or 3
        uint8_t randomRetriedDelay = COAP_ACK_TIMEOUT + intrand((COAP_ACK_TIMEOUT * COAP_RANDOM_FACTOR) - COAP_ACK_TIMEOUT);
        //double maxTimeOut = COAP_ACK_TIMEOUT * COAP_RANDOM_FACTOR;
        //uint8_t randomRetriedDelay = pow(maxTimeOut, retries + 1) + 0.5;
        return randomRetriedDelay;

    }
    /*
     *  Sets the timer field in the metadata buffer.
     *  It calculates a random timeout between COAP_ACK_TIMEOUT and
     *  COAP_RANDOM_FACTOR and sets it to the power of the number of
     *  transmissions (which equals the retries + 1).
     *
     *  You need to update the number of retries BEFORE updating the timer,
     *  else you will end up with a smaller timeout or, if the retries weren't
     *  set yet, with an undefined, possibly very high timeout.
     */
    void setMessageTimer() {
        metaData.timer = calculateTimer(getMessageRetries());
    }
    void setMessageTimer(uint8_t timer) {
        metaData.timer = timer;
    }
    void incrementMessageTimer(int incrementBy) {
        uint8_t result;
        int currTimer = metaData.timer;
        if(incrementBy < 0 && ((currTimer + incrementBy) < 0)) {
            result = 0;
        } else {
            result = currTimer + incrementBy;
        }

        metaData.timer = result;
    }

    void incrementMessageRetries(int incrementBy) {
        uint8_t result;
        if(incrementBy < 0 && (metaData.retries + incrementBy) < 0) {
            result = 0;
        } else {
            result = metaData.retries + incrementBy;
        }

        metaData.retries = result;
    }

    void setMessageTag(uint8_t tag) {
        metaData.tag = tag;
    }
    void setMessageID(uint16_t messageID) {
        uint8_t data[2];
        data[0] = messageID>>8;
        data[1] = messageID&0xFF;
        biPacket->copyToBuffer(data, 2, 2);
    }

    void setMessageSrcPort(uint16_t srcPort) {
        metaData.srcPort = srcPort;
    }
    void setMessageDstPort(uint16_t dstPort) {
        metaData.dstPort = dstPort;
    }
    void setMessageRetries(uint8_t retries) {
        metaData.retries = retries;
    }
    void setMessageOptionsLen(uint16_t optionsLen) {
        metaData.optionsLength = optionsLen;
    }
    void setMessagePayloadLen(uint16_t payloadLen) {
        metaData.payloadLength = payloadLen;
    }
    void setMessageError(uint8_t error) {
        metaData.error = error;
    }
    void addMessageError(uint8_t error) {
        metaData.error = metaData.error | error;
    }

    bool insertNextOption(
            CoAPMessageOption** opts,
            uint8_t optsMaxLen,
            CoAPMessageOption* insertMe);

    int decodeCoAPMessage(CoAPMessageOption** opts, uint8_t optsMaxLen);
    void encodeCoAPMessage(uint8_t messageType, uint8_t messageCode,
            uint16_t messageID,CoAPMessageOption** options, uint8_t optionsLen,
            Token token, const char* payload, uint16_t payloadLen,
            bool handlePayload);
    ErrorCodes_t decodeCoAPMessageOptions(uint16_t& skip, uint16_t prevOptionDelta,
            CoAPMessageOption** opts, uint8_t optsMaxLen);
    void encodeCoAPMessageOptions(CoAPMessageOption** opts, uint8_t optsMaxLen,
            uint16_t shift);

    uint16_t getEncodedCoAPMessageOptionsLen(CoAPMessageOption** opts,
            uint8_t optsMaxLen);
    bool deleteCoAPPacketOptions(CoAPMessageOption** opts, uint8_t optsMaxLen);

    CoAPMessageOption* decodeMessageOption(uint16_t delta, uint16_t pos, uint16_t size) {
        char optionValue[size];

        memcpy(optionValue, &biPacket->getContent()[pos], size);

        CoAPMessageOption* ret = new CoAPMessageOption(
                this->buffer, delta, optionValue, size, 0);
        if (ret->isValid()) {
            return ret;
        }
        delete ret;
        return NULL;

    }

    ErrorCodes_t decodeMessageOptionsMetadata(uint16_t& optionDelta, uint16_t& optionLen, uint16_t& skip) {
        optionDelta = ( biPacket->getContent()[skip] >> 4) & 0xF;
        optionLen =     biPacket->getContent()[skip++]     & 0xF;

        if(optionDelta == 13) {
            optionDelta += biPacket->getContent()[skip++];
        } else if(optionDelta == 14) {
            optionDelta += (((uint16_t)(biPacket->getContent()[skip + 1])<<8) |
                    biPacket->getContent()[skip]) + 269;
            skip += 2;
        } else if(optionDelta == 15) {
            if(optionLen == 15) {   //Payload Marker found
                return OPT_SUCC;
            } else {                //No payload marker found -> Message Format Error
                return OPT_MESSAGE_FORMAT_ERROR;
            }
        }

        if(optionLen == 13)
        {
            optionLen += biPacket->getContent()[skip++];
        }
        else if(optionLen == 14)
        {
            optionLen += ((biPacket->getContent()[skip+1]<<8) |
                    biPacket->getContent()[skip]) + 269;
            skip += 2;
        }
        else if(optionLen == 15)
            return OPT_MESSAGE_FORMAT_ERROR;

        setMessageOptionsLen(getMessageOptionsLen() + optionLen + 1);

        return OPT_GO_ON;
    }

    CoAPBuffer_t*       buffer;
    BufferInformation*  biPacket;

    struct {
        uint8_t     error = 0;
        uint8_t     tag;
        uint8_t     timer;
        uint8_t     retries;
        uint16_t    srcPort;
        uint16_t    dstPort;
        uint16_t    optionsLength;
        uint16_t    payloadLength;
    } metaData;

    IPv6Address         addr;
};

}

#endif /* COAPMESSAGE_H_ */
