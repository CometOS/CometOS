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
 * @file main.cc Testing flash memory in async mode
 * @author Andreas Weigel
 */
#include "cometos.h"
#include "logging.h"
#include "Module.h"
#include "palLed.h"
#include "s25fl_async.h"
#include <util/delay.h>

using namespace cometos;

void doFlashStuff();
void writeResultsToUart();

#define BUFSIZE 256

class AsyncFlashMemTest;

void flashCb(cometos_error_t result);

AsyncFlashMemTest* &getInstance() {
    static AsyncFlashMemTest* afmt = NULL;
    return afmt;
}

class AsyncFlashMemTest : public cometos::Module {
public:
    static const uint8_t ERASE = 4;
    static const uint8_t WRITE = 2;
    static const uint8_t READ  = 1;
    static const uint8_t UART = 8;

    AsyncFlashMemTest() :
        status(0xAB),
        result(COMETOS_SUCCESS),
        mode(WRITE|ERASE|READ)
    {}

    virtual void initialize() {
        // init leds and flash mem
        if (s25fl_init() != COMETOS_SUCCESS) {
           error(1);
        }

        getInstance() = this;
        if(mode & ERASE) {
            currOp = ERASE;
    	    cometos::getCout() << "doErase scheduled" << cometos::endl;
            schedule(&doStuff, &AsyncFlashMemTest::doErase, 0);
        } else if (mode & WRITE) {
            currOp = WRITE;
            schedule(&doStuff, &AsyncFlashMemTest::doWrite, 0);
        } else if (mode & READ) {
            currOp = READ;
            schedule(&doStuff, &AsyncFlashMemTest::doRead, 0);
        }
    }

    void error(uint8_t val) {
        while(1) {
            switch(val) {
            case 1:_delay_ms(100); break;
            case 2:_delay_ms(500); break;
            case 3:_delay_ms(1000); break;
            }
            palLed_toggle(4);
        }
    }


    void flashCallback(cometos_error_t result) {
        if (result != COMETOS_SUCCESS) {
            if (result == COMETOS_ERROR_BUSY) {
                error(2);
            } else {
                error(3);
            }
        }
        switch (currOp) {
            case ERASE:{
                if (mode & WRITE) {
                    currOp = WRITE;
                    schedule(&doStuff, &AsyncFlashMemTest::doWrite, 0);
                } else if (mode & READ) {
                    currOp = READ;
                    schedule(&doStuff, &AsyncFlashMemTest::doRead, 0);
                }
            } break;
            case WRITE:{
                if (mode & READ) {
                    currOp = READ;
                    schedule(&doStuff, &AsyncFlashMemTest::doRead, 0);
                }
            } break;
            case READ: {
                currOp = UART;
                schedule(&readDone, &AsyncFlashMemTest::writeResultsToUart, 0);
            }
        }
    }

    void getFlashState() {
        _delay_ms(1000);
        result = s25fl_readStatus(&status);
        cometos::getCout() << "op=" << (int) currOp  << "|r=" << (int) result << "|fs=" << (int) status << cometos::endl;
        if (result != COMETOS_SUCCESS){
           error(1);
        }
    }

    void doWrite(cometos::Message* msg) {
        getFlashState();
        result = s25fl_pageProgramAsync(0x1e, tx, sizeof(tx), flashCb);
        if (result != COMETOS_SUCCESS) {
            error(2);
        }
    }

    void doErase(cometos::Message* msg) {
	    cometos::getCout() << "Erase" << cometos::endl;

        getFlashState();
        result = s25fl_sectorEraseAsync(0x000000, flashCb);
        if (result != COMETOS_SUCCESS){
            error(3);
        }
    }

    void doRead(cometos::Message* msg) {
        getFlashState();

        result = s25fl_readAsync(0, rx, BUFSIZE, flashCb);
        if (result != COMETOS_SUCCESS) {
           error(2);
        }
    }


    void writeResultsToUart(cometos::Message* msg) {
        getFlashState();
        for (int i = 0; i < BUFSIZE; i++) {
            if (i % 16 == 0 && i != 0){
                cometos::getCout() << cometos::endl;
            }
            cometos::getCout() << cometos::hex << (int) rx[i] << " ";
        }
        cometos::getCout() << cometos::endl << cometos::endl;
    }

private:
    uint8_t rx[BUFSIZE];
    uint8_t tx[BUFSIZE] = {'a', 'b', 'c', 28, 27, 26, 25, 24, 23, 22, 21, 20, 'x', 'y', 'z'};
    uint8_t status;
    uint8_t result;
    uint8_t mode;
    uint8_t currOp;

    cometos::Message readDone;
    cometos::Message doStuff;
};





void flashCb(cometos_error_t result) {
    getInstance()->flashCallback(result);
}


static AsyncFlashMemTest afmt;

int main() {
    cometos::getCout() << "Starting async flash test program" << cometos::endl;

    cometos::initialize();
    cometos::getCout() << "After initialize" << cometos::endl;
    cometos::run();
}





