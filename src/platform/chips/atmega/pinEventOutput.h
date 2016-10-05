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
 * CometOS Platform Abstraction Layer Initialization
 *
 * @author Andreas Weigel
 */

#ifndef PIN_EVENT_OUTPUT_H
#define PIN_EVENT_OUTPUT_H

#ifdef PIN_EVENT_OUTPUT
#include <avr/io.h>

// for pin event outputs
enum {
     PEO_RADIO_RX_DROPPED = 0,
     PEO_TX_BO_FAIL = 2,
     PEO_RADIO_TX_START = 3,
     PEO_RADIO_RX_START = 4,
     PEO_RADIO_TX_END = 5,
     PEO_TX_RETRY = 6,
     PEO_RADIO_RX_END = 7,
     PEO_TX_REQUEST = 9,
     PEO_RX_DROPPED = 10,
     PEO_TX_START = 11,
     PEO_RX_DONE = 12,
     PEO_TX_SUCCESS = 13,
     PEO_TX_FAIL = 14
};

#define DDR(x) (*(&(x) - 1))
#define EVENT_OUTPUT_WRITE(s) EVENT_OUTPUT_PORT = ((EVENT_OUTPUT_PORT & ~EVENT_OUTPUT_MASK) | ((s)<<EVENT_OUTPUT_SHIFT))
#define EVENT_OUTPUT_INIT() DDR(EVENT_OUTPUT_PORT) |= EVENT_OUTPUT_MASK; EVENT_OUTPUT_PORT &= ~EVENT_OUTPUT_MASK;
#else
#define EVENT_OUTPUT_WRITE(s)
#define EVENT_OUTPUT_INIT()
#endif



#endif
