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

/*INCLUDES-------------------------------------------------------------------*/
 
#include "cometos.h"
 
#include "palLed.h"

#include "Module.h"
#include "Queue.h"
//#include "palSerial.h"
#include "palExec.h"
#include "palLocalTime.h"
#include "MacSymbolCounter.h"
#include "rf231.h"
using namespace cometos;

#include "PureSniffer.h"

//PalSerial* serial;

class OutputBuffer {
public:
    char output[129*2+3];
    uint8_t x;
};

#define BUFFERS 100
OutputBuffer buffers[BUFFERS];
Queue<OutputBuffer*,BUFFERS> waiting;
Queue<OutputBuffer*,BUFFERS> empty;

inline char numToHex(uint8_t num) {
    if(num <= 9) {
      return num+'0';
    }
    else {
      return num-10+'A';
    }
}

void push(uint8_t* data, uint8_t length, uint32_t sfdTimestamp) {
    palExec_atomicBegin();
    ASSERT(!empty.empty());
    OutputBuffer* b = empty.front();
    empty.pop();
    palExec_atomicEnd();

    b->output[b->x++] = 'P';
    b->output[b->x++] = 'K';
    b->output[b->x++] = 'T';

    for(int i = 0; i < 8; i++) {
        b->output[b->x++] = numToHex((sfdTimestamp >> 28) & 0xF);
        sfdTimestamp <<= 4;
    }

    b->output[b->x++] = ':';

    for(int i = 0; i < length; i++) {
        uint8_t v = data[i];
        b->output[b->x++] = numToHex(v >> 4);
        b->output[b->x++] = numToHex(v & 0xF);
    }
    b->output[b->x++] = '\n';
    
    palExec_atomicBegin();
    ASSERT(waiting.push(b));
    palExec_atomicEnd();
}

void print();
SimpleTask printTask(print);
void print() {
    palExec_atomicBegin();
    while(!waiting.empty()) {
        OutputBuffer* b = waiting.front();
        waiting.pop();

        palExec_atomicEnd();
        //serial->write(b->output,b->x);
        b->output[b->x++] = '\0';
        getCout() << b->output;
        b->x = 0;
        palExec_atomicBegin();

        empty.push(b);
    }
    palExec_atomicEnd();

    getScheduler().add(printTask);
}

int main() {
    initialize();

    for(int i = 0; i < BUFFERS; i++) {
        empty.push(&buffers[i]);
    }

    //serial = PalSerial::getInstance<int>(1);
	
    auto& sniffer = PureSniffer::getInstance();
    sniffer.init(CALLBACK_FUN(push));

    //serial->write((const uint8_t*)"BOOT\n",5);
    getCout() << "BOOTING" << endl;

    getScheduler().add(printTask);

    run();
	
    return 0;
}
