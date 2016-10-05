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


#include "LowpanBuffer.h"

#ifndef COAPCONFIG_H
#define COAPCONFIG_H

namespace cometos_v6 {

const uint8_t COAP_MESSAGE_METADATA_SIZE =  14;
const uint8_t COAP_MAX_MESSAGE_OPTIONS =    15;

const uint8_t COAP_MAX_MESSAGE_BUFFER = 25;
const uint8_t COAP_MAX_RESOURCES = 5;
const uint8_t COAP_MAX_LISTENERS = 5;

const uint16_t COAP_SET_BUFFER_SIZE = 2500;
const uint8_t COAP_SET_BUFFER_ENTRIES = (COAP_MAX_MESSAGE_BUFFER * 3) + COAP_MAX_MESSAGE_OPTIONS;
//typedef LowpanBuffer<COAP_SET_BUFFER_SIZE, COAP_SET_BUFFER_ENTRIES> CoAPBuffer_t;
typedef ManagedBuffer CoAPBuffer_t;

const uint8_t COAP_RETRANSMIT_INTVAL = 2;

static const uint8_t COAP_MESSAGE_LIFETIME = 30;

}

#endif
