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

#include "CoAPMessage.h"
#include "CoAPMessageOption.h"
#include "palLed.h"

using namespace cometos;

namespace cometos_v6 {

// private:
CoAPMessage::CoAPMessage(CoAPBuffer_t* buffer, const IPv6Address& addr,
        uint16_t srcPort, uint16_t dstPort, const uint8_t* packet,
        uint16_t packetLen) :
    buffer(buffer),
    biPacket(NULL),
    addr(addr)
{
    biPacket = buffer->getBuffer(packetLen);
    if(biPacket == NULL){
        this->setMessageError(COAP_MESSAGE_SILENTIGNORE);
        return;
    }
    biPacket->copyToBuffer(packet, packetLen, 0);
    setMessageSrcPort(srcPort);
    setMessageDstPort(dstPort);
    setMessageRetries(0);
    setMessageOptionsLen(0);
    setMessagePayloadLen(0);
    setMessageError(0);
    setMessageTimer();
    setMessageTag(0);

};

// public:
CoAPMessage::CoAPMessage(CoAPBuffer_t* buffer) :
        buffer(buffer), biPacket(NULL)
{
    setMessageSrcPort(0);
    setMessageDstPort(0);
    setMessageRetries(0);
    setMessageOptionsLen(0);
    setMessagePayloadLen(0);
    setMessageError(0);
    setMessageTimer();
    setMessageTag(0);

};

CoAPMessage::~CoAPMessage(){
    if(biPacket != NULL){
        biPacket->free();
        biPacket = NULL;
    }

//        if(addr != NULL)
//            delete addr;
};

SString<5> getMessageCodeString(uint8_t code){

    SString<5> ret;

    ret.append((uint16_t) (code>>5));
    ret += '.';
    ret.append((uint16_t) (code&0x1F), cometos::SString_DEC, 2);

    return ret;
}

SString<255> CoAPMessage::getMessageString() {
    SString<255> ret = "coap://[";
    ret += addr.str();
    ret += "]:";
    ret.append(getMessageDstPort());
    ret += ' ';
    ret.append((uint16_t)getMessageVersion());
    ret += ' ';

    switch(getMessageType()){
        case COAP_CON:  ret += "CON "; break;
        case COAP_NON:  ret += "NON "; break;
        case COAP_ACK:  ret += "ACK "; break;
        case COAP_RST:  ret += "RST "; break;
    }

    ret += '[';
    ret += getMessageCodeString(getMessageCode());
    ret += "] [mid:";
    ret.append(getMessageID(), cometos::SString_HEX, 4);
    ret += "] [tok:";
    for (uint8_t i = 0; i < 7; i++) {
        uint16_t x = getMessageToken().getTokenPart(i);
        if (x > 0) {
            ret.append(x, cometos::SString_HEX, 2);
        }
    }
    ret.append((uint16_t)(getMessageToken().getTokenPart(7)), cometos::SString_HEX, 2);
    ret += ']';

    if(getMessagePayloadLen() > 0) {
        ret += " PLen:";
        ret.append(getMessagePayloadLen());
    }

    if(getMessageError()) {
        ret.append(getErrorCodeDescriptions(getMessageError()));
    }

    return ret;
}

cometos::SString<100> CoAPMessage::getErrorCodeDescriptions(uint8_t error){
    cometos::SString<100> descr = "CoAP MsgErr: ";

    if(error&COAP_MESSAGE_FORMATERROR) {
        descr.append("Msg Format Err\n");
    }

    if(error&COAP_MESSAGE_SILENTIGNORE) {
        descr.append("Silently ignore this error\n");
    }

    if(error&COAP_MESSAGE_UNREC_OPTION) {
        descr.append("Unrec option found\n");
    }

    if(error&COAP_MESSAGE_URL_UNREC_PREFIX) {
#ifdef COAPS
        descr.append("Unrec prefix (use coap:// or coaps://)\n");
#else
        descr.append("Unrec prefix (use coap://)\n");
#endif
    }

    if(error&COAP_MESSAGE_URL_UNREC_ADDRESS) {
        descr.append("Invld IPv6 Address\n");
    }

    if(error&COAP_MESSAGE_URL_UNREC_PATH) {
        descr.append("Invld URI Path/Query \n");
    }

    if(error&COAP_MESSAGE_ILLEGAL_TYPE_OR_CODE) {
        descr.append("Msg Type or Code is invalid\n");
    }

    if(error&COAP_MESSAGE_QUEUE_FULL) {
       descr.append("The resend queue is full. Please wait.\n");
    }

    if(error == 0) {
        descr.append("No errors occured\n");
    }

    return descr;
}

/*
 * This init function gets its message id and token from the caller.
 */

int8_t CoAPMessage::init(
        const URL& url, uint16_t srcPort, uint8_t messageType, uint8_t messageCode,
        uint16_t messageID, Token token, CoAPMessageOption** options,
        uint8_t optionsLen, const char* payload, uint16_t payloadLen,
        bool handleOptions, bool handlePayload)
{

    setMessageSrcPort(srcPort);
    setMessagePayloadLen(payloadLen);

    int8_t err = setURL(url, options, optionsLen);
    if (err < 0) {
        if(handleOptions) {
            deleteCoAPPacketOptions(options, optionsLen);
        }
        setMessageError(err);
        return err;
    }

    encodeCoAPMessage(
            messageType, messageCode, messageID, options, optionsLen, token,
            payload, payloadLen, handlePayload);

    if(handleOptions) {
        deleteCoAPPacketOptions(options, optionsLen);
    }

    return getMessageError();
}

int8_t CoAPMessage::setURL(const URL& url, CoAPMessageOption** options,
        uint8_t optionsMaxLen) {
    if (url.protocol != URL::COAP) {
        return OPT_UNKNOWN_PROTOCOL;
    }

    addr = url.ip;
    if (url.port == 0) {
        setMessageDstPort(COAP_PORT);
    } else {
        setMessageDstPort(url.port);
    }

    uint16_t optionOrder = 0;

    for (uint8_t i = 0; i < url.numParts; i++) {
        if (url.uriParts[i].getType() == URIPart::URIPATH) {
            if(i > 0 && url.uriParts[i-1].getType() == URIPart::URIPATH) {
                optionOrder++;
            }

            CoAPMessageOption* opt = new CoAPMessageOption(
                    this->buffer,
                    CoAPMessageOption::OPT_URIPATH,
                    url.uriParts[i].getPart(), url.uriParts[i].getLength(),
                    optionOrder);
            if (!opt->isValid() || !insertNextOption(options, optionsMaxLen, opt)) {
                delete opt;
                return OPT_UNKNOWN_ERROR;
            }
        } else if (url.uriParts[i].getType() == URIPart::URIQUERY) {
            if(i > 0 && url.uriParts[i-1].getType() == URIPart::URIPATH) {
                optionOrder = 0;
            } else if(i > 0 && url.uriParts[i-1].getType() == URIPart::URIQUERY) {
                optionOrder++;
            }

            CoAPMessageOption* opt = new CoAPMessageOption(
                    this->buffer,
                    CoAPMessageOption::OPT_URIQUERY,
                    url.uriParts[i].getPart(), url.uriParts[i].getLength(),
                    optionOrder);
            if(!opt->isValid() || !insertNextOption(options, optionsMaxLen, opt)) {
                delete opt;
                return OPT_UNKNOWN_ERROR;
            }
        } else {
            return OPT_UNREC_OPTION;
        }
    }

    return 0;

}

/**
 * This init functions gets a new message id and a new token itself
 */
/*
int CoAPMessage::init(
        string url, uint16_t srcPort, uint8_t messageType, uint8_t messageCode,
        List<CoAPPacketOption*>* options, uint8_t* payload, uint16_t payloadLen,
        bool handleOptions, bool handlePayload)
{

    List<CoAPPacketOption*> opts;

    List<CoAPPacketOption*>::iterator i;

    if(options != NULL)
        while (options->size() != 0)
        {
            i = options->begin();
            opts.push_back(new CoAPPacketOption(buffer, (*i)));
            options->pop_front();
        }

    setMessageSrcPort(srcPort);
    setMessagePayloadLen(payloadLen);

    parseURL(url, &opts);
    if(getMessageError())
        return getMessageError();

  //  uint64_t token = 0;



    encodeCoAPMessage(messageType, messageCode, messageID, &opts, token, payload, payloadLen, handlePayload);

    deleteCoAPPacketOptions(&opts);

    return getMessageError();
}
*/

bool CoAPMessage::insertNextOption(
        CoAPMessageOption** opts, uint8_t optsMaxLen,
        CoAPMessageOption* insertMe)
{
    for(uint8_t i = 0; i < optsMaxLen; i++) {
        if(opts[i] == NULL){
            opts[i] = insertMe;
            return true;
        }
    }
    return false;
}

int CoAPMessage::getMessageCodeMeaning(codeMeaning_t& meaning){


    uint8_t messageCode = getMessageCode();

    if(messageCode == COAP_EMPTY) {
        //0.00
        meaning = EMPTY;
    } else if((messageCode & 0xE0) == 0) {
        //0.00-1.00 -> 000.00000-001.00000 -> 0x00-0x20
        meaning = REQUEST;
    } else if((messageCode & 0x60) != 0x60) {
        //2.00-2.31 -> 010.00000-010.11111 -> 0x40-0x5F
        meaning = RESPONSE;
        //4.00-5.31 -> 100.00000-101.11111 -> 0x80-0x9F
    } else {
        //else it uses a reserved message code (1.00-1.31 // 3.00-3.31 // 6.0-7.31)
        meaning = ERROR;
        return COAP_MESSAGE_FORMATERROR;
    }

    return 0;
}

/*
 * CoAP Message Format:
 *
 *                       1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |Ver| T |  TKL  |      Code     |          Message ID           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |   Token (if any, TKL bytes) ...                               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |   Options (if any) ...                                        |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |1 1 1 1 1 1 1 1|    Payload (if any) ...                       |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 */

int CoAPMessage::decodeCoAPMessage(
        CoAPMessageOption** opts, uint8_t optsMaxLen)
{

    uint16_t skip = 0;
    codeMeaning_t meaning;
    uint8_t messageType = getMessageType();

    if(getMessageVersion() != COAP_VERSION){        //If the message uses a different CoAP Version
        addMessageError(COAP_MESSAGE_SILENTIGNORE);
        return COAP_MESSAGE_SILENTIGNORE;           //then silently ignore the message.
    }
    uint8_t tokenLen = getMessageTokenLen();        //Parse token length
    if(tokenLen > 8){                               //If token is too long
        addMessageError(COAP_MESSAGE_FORMATERROR);
        return COAP_MESSAGE_FORMATERROR;            //return MESSAGE_FORMAT_ERROR
    }

    int error = getMessageCodeMeaning(meaning);
    if(messageType == COAP_RST){
        if(getMessageTokenLen() > 0 || getMessagePacketLen() > 4 || meaning == REQUEST || meaning == RESPONSE)
            error++;
    }
    else if(messageType == COAP_CON){
        if(meaning == EMPTY) {
            //Combinations that trigger MESSAGE_FORMAT_ERROR's
            error++;
        }
    }
    else if(messageType == COAP_NON){
        if(meaning == EMPTY) {
            error++;
        }
    }
    else if(messageType == COAP_ACK){
        if(meaning == REQUEST) {
            error++;
        }
    }
    if(error){
        addMessageError(COAP_MESSAGE_FORMATERROR);
        return COAP_MESSAGE_FORMATERROR;
    }
    skip = 4 + tokenLen;

    //Initiating variables needed for decoding the options
    uint16_t initialOptionDelta=0;

    if(skip >= getMessagePacketLen()){
        if(opts != NULL) {
            memset(opts, 0, sizeof(CoAPMessageOption*)*optsMaxLen);
        }

        setMessagePayloadLen(0);
        return COAP_MESSAGE_OK;
    }

    ErrorCodes_t optionsErrCode = decodeCoAPMessageOptions(
            skip, initialOptionDelta, opts, optsMaxLen);     //Parse the Message Options
    if(optionsErrCode != OPT_SUCC){
        if(opts != NULL) {
            memset(opts, 0, sizeof(CoAPMessageOption*)*optsMaxLen);
        }
    }
    if(optionsErrCode==OPT_UNREC_OPTION){
        addMessageError(COAP_MESSAGE_UNREC_OPTION);
        return COAP_MESSAGE_UNREC_OPTION;
    }
    else if(optionsErrCode == OPT_MESSAGE_FORMAT_ERROR){
        addMessageError(COAP_MESSAGE_FORMATERROR);
        return COAP_MESSAGE_FORMATERROR;
    }

    if(skip == getMessagePacketLen()){
        setMessagePayloadLen(0);
        return COAP_MESSAGE_OK;
    }

    setMessagePayloadLen(getMessagePacketLen() - skip);


    return getMessageError();
}

void bubbleSort(CoAPMessageOption** array, uint8_t length)
{
    CoAPMessageOption* temp;
    uint8_t test; /*use this only if unsure whether the list is already sorted or not*/
    for(uint8_t i = length - 1; i > 0; i--)
    {
        test=0;
        for(uint8_t j = 0; j < i; j++)
        {
            if(array[j + 1] != NULL) {   //Check, if the last element is reached.
                if((array[j]->getOptionNr() > array[j+1]->getOptionNr()) ||
                        (array[j]->getOptionNr() == array[j+1]->getOptionNr() &&
                                array[j]->getOptionOrder() > array[j+1]->getOptionOrder()))
                    /* compare neighboring elements */
                {
                    temp = array[j]; /* swap array[j] and array[j+1] */
                    array[j] = array[j+1];
                    array[j+1] = temp;
                    test=1;
                }
            } else {
                break;
            }
        } /*end for j*/
        if(test==0) break; /*will exit if the list is sorted!*/
    } /*end for i*/

}

uint16_t CoAPMessage::getEncodedCoAPMessageOptionsLen(
        CoAPMessageOption** opts, uint8_t optsMaxLen)
{

    uint16_t lastOptionNr = 0;
    uint16_t currOptionDelta = 0;
    uint16_t currOptionLen = 0;

    uint16_t skip = 0;
    uint16_t addedOptionLen = 0;
    bubbleSort(opts, optsMaxLen);

    for (uint8_t i = 0; i < optsMaxLen && opts[i] != NULL; ++i) //Iterate through list and calculate needed buffer size
    {
        currOptionDelta = opts[i]->getOptionNr() - lastOptionNr;
        currOptionLen = opts[i]->getOptionLen();
        lastOptionNr += currOptionDelta;

        if(currOptionDelta > 12) {
            if(currOptionDelta < 269) {
                skip += 1;
            } else {
                skip += 2;
            }
        }

        if(currOptionLen > 12) {
            if(currOptionLen < 269) {
                skip += 1;
            } else {
                skip += 2;
            }
        }

        addedOptionLen += currOptionLen;
        addedOptionLen += 1;            //Each Option has at least 1B, the option delta and the option len.
    };
    return addedOptionLen +  skip;
}

void CoAPMessage::encodeCoAPMessage(
        uint8_t messageType, uint8_t messageCode, uint16_t messageID,
        CoAPMessageOption** options, uint8_t optionsLen, Token token,
        const char* payload, uint16_t payloadLen, bool handlePayload)
{
    uint16_t shift=0;

    if(options != NULL && optionsLen != 0) {
        setMessageOptionsLen(getEncodedCoAPMessageOptionsLen(options,
                optionsLen));
    } else {
        setMessageOptionsLen(0);
    }

    uint8_t tokenLen = token.getTokenLength();
    uint16_t messageLen = 4 + getMessageOptionsLen() + tokenLen +
            getMessagePayloadLen();
    if(payloadLen) {
        messageLen++;
    }

    biPacket = buffer->getBuffer(messageLen);

    if(biPacket == NULL){
        setMessageError(COAP_MESSAGE_SILENTIGNORE);
        return;
    }

    uint8_t header[4];
    header[shift] = ( (COAP_VERSION<<6) | ((messageType<<4)&0x30) ) | (tokenLen&0x0F);
    header[++shift] = messageCode;
    header[++shift] = (messageID>>8)&0xFF;
    header[++shift] = messageID&0xFF;

    biPacket->copyToBuffer(header, 4, 0);

    for(uint8_t i = 0; i < tokenLen; i++){
        uint8_t tokenTemp = token.getTokenPart(i + (Token::MAX_LENGTH - tokenLen));
        biPacket->copyToBuffer(&tokenTemp, 1, ++shift);
    }

    if(getMessageOptionsLen()){
        encodeCoAPMessageOptions(options, optionsLen, ++shift);
        shift += getMessageOptionsLen() - 1;
    }

    if(payloadLen && payload!=NULL){

        uint8_t temp = 0xFF;
        shift++;
        biPacket->copyToBuffer(&temp, 1, shift);            //Insert payload indicator
        shift++;
        biPacket->copyToBuffer((uint8_t*)payload, payloadLen, shift); //Insert payload indicator
        shift+=payloadLen;

        if(handlePayload) {
            delete payload;
        }
    }

}

int CoAPMessage::findInCoAPMessageOptions(
        uint16_t optionNr, CoAPMessageOption** opts, uint8_t optsSize)
{

    CoAPMessageOption* tempOptions[COAP_MAX_MESSAGE_OPTIONS];
    memset(tempOptions, 0, sizeof(tempOptions));
    uint8_t error = decodeCoAPMessageOptions(tempOptions, COAP_MAX_MESSAGE_OPTIONS);

    if(error != 0) {
        return 0;
    }
    uint8_t i, j;
    for(i = 0, j = 0; i < COAP_MAX_MESSAGE_OPTIONS && j < optsSize; i++)
    {
        if(tempOptions[i] != NULL && tempOptions[i]->getOptionNr() == optionNr)
        {
            opts[j] = tempOptions[i];
            j++;
        }
        else if(tempOptions[i] != NULL) {
            delete tempOptions[i];
        }
    }

    return j;
}


int CoAPMessage::decodeCoAPMessageOptions(
        CoAPMessageOption** opts, uint8_t optsSize)
{
    uint16_t optionDelta = 0;
    uint16_t prevOptionDeltas = 0;
    setMessageOptionsLen(0);

    uint16_t skip = 4 + getMessageTokenLen();

    for(uint8_t i = 0; i < optsSize && skip < this->getMessagePacketLen(); i++)
    {
        uint16_t optionLen = 0;

        switch (decodeMessageOptionsMetadata(optionDelta, optionLen, skip)) {
            case OPT_SUCC:
                return 0;
            case OPT_MESSAGE_FORMAT_ERROR:
                for (uint8_t j = 0; j < i; j++) {
                    delete opts[j];
                }
                return COAP_MESSAGE_FORMATERROR;
            default:
                ;
        }

        if(opts != NULL){
            opts[i] = decodeMessageOption(optionDelta + prevOptionDeltas,
                    skip, optionLen);
            if (opts[i] == NULL) {
                for (uint8_t j = 0; j < i; j++) {
                    delete opts[j];
                }
                return 4; // TODO: ???? Warum 4? Darum
            }
            if(opts[i]->getData() == NULL) {
                for (uint8_t j = 0; j < i; j++) {
                    delete opts[j];
                }
                return 5;
            }
        }

        prevOptionDeltas += optionDelta;

        skip += optionLen;
    }
    return 0;
}

CoAPMessage::ErrorCodes_t CoAPMessage::decodeCoAPMessageOptions(
        uint16_t& skip, uint16_t prevOptionDelta, CoAPMessageOption** opts,
        uint8_t optsMaxLen)
{
    uint16_t optionDelta = 0;
    uint16_t optionLen = 0;
    setMessageOptionsLen(0);

    while(skip < getMessagePacketLen())
    {
        switch (decodeMessageOptionsMetadata(optionDelta, optionLen, skip)) {
            case OPT_SUCC:
                return OPT_SUCC;
            case OPT_MESSAGE_FORMAT_ERROR:
                return OPT_MESSAGE_FORMAT_ERROR;
            default:
                ;
        }

        if(opts != NULL){
            CoAPMessageOption* tempOption = decodeMessageOption(
                    optionDelta, skip, optionLen);
            if(!insertNextOption(opts, optsMaxLen, tempOption)){
                for(uint8_t i = 0; i < optsMaxLen; i++) {
                    if(opts[i] != NULL){
                        delete opts[i];
                        opts[i] = NULL;
                    }
                }
                return OPT_UNREC_OPTION;
            }
        }

        skip += optionLen;
    };
    return OPT_SUCC;
}

void CoAPMessage::encodeCoAPMessageOptions(
        CoAPMessageOption** opts, uint8_t optsMaxLen, uint16_t shift)
{
    uint8_t optionData[getMessageOptionsLen()];

    uint16_t lastOptionNr = 0;
    uint16_t currOptionDelta = 0;
    uint16_t currOptionLen = 0;

    uint16_t skip = 0;

    bubbleSort(opts, optsMaxLen);

    lastOptionNr = 0;
    for (uint8_t i = 0; i < optsMaxLen && opts[i] != NULL; ++i) //encode the options into the allocated buffer.
    {
        uint16_t currOptionBegin=skip;

        currOptionDelta = opts[i]->getOptionNr() - lastOptionNr;
        currOptionLen = opts[i]->getOptionLen();
        lastOptionNr += currOptionDelta;

        if(currOptionDelta < 13) {                               //encode optionNr
            optionData[currOptionBegin] = (currOptionDelta & 0xF) << 4;
        } else if(currOptionDelta<269) {
            optionData[currOptionBegin] = 13 << 4;
            optionData[++skip] = ((currOptionDelta - 13) & 0xFF);
        }
        else
        {
            optionData[currOptionBegin] = 14 << 4;
            optionData[++skip] = ((currOptionDelta - 269) >> 8) & 0xFF;
            optionData[++skip] = (currOptionDelta - 269) & 0xFF;
        }

        if(currOptionLen < 13) {                      //encode optionLen
            optionData[currOptionBegin] |= currOptionLen & 0xF;
        } else if(currOptionLen < 269) {
            optionData[currOptionBegin] |= 13;
            optionData[++skip] = ((currOptionLen - 13) & 0xFF);
        }
        else
        {
            optionData[currOptionBegin] |= 14;
            optionData[++skip] = ((currOptionLen - 269) >> 8) & 0xFF;
            optionData[++skip] = ((currOptionLen - 269) & 0xFF);
        }

        memcpy(&optionData[++skip],
                opts[i]->getOptionData(),
                opts[i]->getOptionLen());       //Finally copy the data.
        skip+=opts[i]->getOptionLen();
        delete opts[i];
        opts[i] = NULL;
    }

    biPacket->copyToBuffer(optionData, getMessageOptionsLen(), shift);

    if(skip!=0) {
        if(getMessageOptionsLen() != skip) {
            return;
        }
    }
    return;
}

bool CoAPMessage::deleteCoAPPacketOptions(
        CoAPMessageOption** opts, uint8_t optsMaxLen)
{

    if(opts != NULL) {

		for(uint8_t i = 0; i < optsMaxLen; i++)
		{
			if(opts[i] != NULL){
				delete opts[i];
				opts[i] = NULL;
			}
		}
    }

    return true;
}

}
