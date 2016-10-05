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
 * @author Stefan Unterschuetz
 */

#include <threadutils.h>

#include <stdio.h>
#include <stdlib.h>
#include "palSerial.h"
#include "rs232.h"
#include <iostream>

#include "logging.h"
#include "Task.h"
#include <map>

namespace cometos {

#ifndef _WIN32
#include <time.h>
#endif

void rxHandle(thread_handler_t, void* p);
void txHandle(thread_handler_t, void* p);


/*MACROS---------------------------------------------------------------------*/
#define TX_BUFFER_SIZE  192

#define RX_BUFFER_SIZE  192


class PalSerialImpl : public PalSerial {
    friend void rxHandle(thread_handler_t, void* p);
    friend void txHandle(thread_handler_t, void* p);
    friend PalSerial* getInstance<const char*>(const char *);

private:
    /*VARIABLES------------------------------------------------------------------*/

    uint8_t rxBuffer[RX_BUFFER_SIZE];
    uint8_t txBuffer[TX_BUFFER_SIZE];
    volatile uint8_t rxLength;
    volatile uint8_t txLength;

    volatile uint8_t rxPos;
    volatile uint8_t txPos;

    Task* txFifoEmptyCallback;
    Task* txReadyCallback;
    Task* rxStartCallback;


    thread_handler_t rxThread;
    thread_handler_t txThread;

    thread_mutex_t rxMutex;
    thread_mutex_t txMutex;

    int hPort;

    std::string port;

    /*CTOR-----------------------------------------------------------------------*/
    PalSerialImpl(std::string port) :
        txFifoEmptyCallback(NULL)
    ,   txReadyCallback(NULL)
    ,   rxStartCallback(NULL)
    ,	port(port)
    {}


public:
    void init(uint32_t baudrate, Task* rxStartCallback,
              Task* txFifoEmptyCallback, Task* txReadyCallback) {

        rxPos = 0;
        txPos = 0;
        rxLength = 0;
        txLength = 0;
        this->txFifoEmptyCallback = txFifoEmptyCallback;
        this->rxStartCallback = rxStartCallback;
        this->txReadyCallback = txReadyCallback;

        rxMutex = thread_createMutex();
        txMutex = thread_createMutex();

        hPort = OpenComport(port.c_str(), baudrate);

        if (-1 == hPort) {
            printf("Unable to open port; aborting\n");
            ASSERT(false);
            return;
        }

        rxThread = thread_run(rxHandle, this);
        txThread = thread_run(txHandle, this);
    }

    uint8_t write(const uint8_t* data, uint8_t length, bool flagFirst) {
        uint8_t i;

        thread_lockMutex(txMutex);
        volatile uint8_t len = txLength;
        volatile uint8_t pos = txPos;
        thread_unlockMutex(txMutex);

        if ((TX_BUFFER_SIZE - len) < length) {
            length = (TX_BUFFER_SIZE - len);
        }

        pos = (pos + len) % TX_BUFFER_SIZE;

        for (i = 0; i < length; i++) {
            txBuffer[pos] = data[i];
            pos = (pos + 1) % TX_BUFFER_SIZE;
        }

        thread_lockMutex(txMutex);
        txLength += length;
        thread_unlockMutex(txMutex);

        return length;
    }

    uint8_t read(uint8_t* data, uint8_t length) {
        uint8_t i;
        thread_lockMutex(rxMutex);
        volatile uint8_t len = rxLength;
        volatile uint8_t pos = rxPos;
        thread_unlockMutex(rxMutex);

        // get maximum number of bytes that can be read
        if (len < length) {
            length = len;
        }

        // get start position for reading
        pos = (RX_BUFFER_SIZE + pos - len) % RX_BUFFER_SIZE;

        for (i = 0; i < length; i++) {
            data[i] = rxBuffer[pos];
            pos = (pos + 1) % RX_BUFFER_SIZE;
        }

        thread_lockMutex(rxMutex);
        rxLength -= length;
        thread_unlockMutex(rxMutex);

        return length;
    }

	void set_9bit_mode(bool enable, bool multiProcessorMode, Callback<bool(uint8_t,bool)> rxByteCallback) {
	}
};


template<> PalSerial* PalSerial::getInstance<const char*>(const char* peripheral) {
    static std::map<std::string, PalSerialImpl*> handles;
    std::string s(peripheral);
    if (handles.find(s) == handles.end()) {
        handles[s] = new PalSerialImpl(s);
    }
    return handles[s];
}



/*FUNCTION DEFINITION--------------------------------------------------------*/

void rxHandle(thread_handler_t h, void *p) {
#ifndef _WIN32
    static struct timespec delay;
    delay.tv_sec = 0;
    delay.tv_nsec = 500000L;
#endif

    PalSerialImpl * psi = static_cast<PalSerialImpl*>(p);
    while (!thread_receivedStopSignal(h)) {
#ifdef _WIN32
        usleep(500);
#else
        nanosleep(&delay, NULL);
#endif
        thread_lockMutex(psi->rxMutex);
        int numFree = RX_BUFFER_SIZE - psi->rxLength;
        int tillEnd = RX_BUFFER_SIZE - psi->rxPos;
        uint8_t * pos = psi->rxBuffer + psi->rxPos;
        uint8_t toRead = tillEnd < numFree ? tillEnd : numFree;
        thread_unlockMutex(psi->rxMutex);

//      printf("read\n");
        int n = PollComport(psi->hPort, pos, toRead);
        if (n < 0) {
            ASSERT(n >= 0);
            getCout() << "Could not poll to port with handle " << psi->hPort << ", aborting" << endl;
            return;
        }

        if (n > 0) {
//            std::cout << "rx:" << (int) n << std::endl;
//          printf("rx: %d bytes\n", n);
        }
//        else {
//            count++;
//            if (count > 1000) {
//                std::cout << "still polling; descr:" << psi->hPort << "|pos=" << (uintptr_t)pos << "|len=" << (int)toRead << std::endl;
//                count = 0;
//            }
//        }
        thread_lockMutex(psi->rxMutex);
        psi->rxPos = (psi->rxPos + n) % RX_BUFFER_SIZE;
        psi->rxLength += n;
        if (psi->rxLength > 0 && psi->rxStartCallback != NULL) {
//            std::cout << "calling cb function; len=" << (int) psi->rxLength << "|ptr=" << (uintptr_t) psi->rxStartCallback << std::endl;
            thread_unlockMutex(psi->rxMutex);
            psi->rxStartCallback->invoke();
            thread_lockMutex(psi->rxMutex);
        }
        thread_unlockMutex(psi->rxMutex);
    }

}

void txHandle(thread_handler_t h, void *p) {
#ifndef _WIN32
    static struct timespec delay;
    delay.tv_sec = 0;
    delay.tv_nsec = 500000L;
#endif
    PalSerialImpl * psi = static_cast<PalSerialImpl*>(p);

    while (!thread_receivedStopSignal(h)) {
#ifdef _WIN32
        usleep(500);
#else
        nanosleep(&delay, NULL);
#endif
        uint8_t * pos;
        uint8_t toWrite;
        thread_lockMutex(psi->txMutex);
        while (psi->txLength > 0) {
            if (psi->txPos + psi->txLength >= TX_BUFFER_SIZE) {
                uint8_t tmpLen = TX_BUFFER_SIZE - psi->txPos;
//                printf("SEND %d|%d\n",txPos, TX_BUFFER_SIZE - txPos);
                pos = psi->txBuffer + psi->txPos;
                toWrite = TX_BUFFER_SIZE - psi->txPos;
                thread_unlockMutex(psi->txMutex);
                SendBuf(psi->hPort, pos, toWrite);
                thread_lockMutex(psi->txMutex);
                psi->txLength -= tmpLen;
                psi->txPos = 0;
            }
            pos = psi->txBuffer + psi->txPos;
            toWrite = psi->txLength;
            // either send "rest" or whole buffered data
//            printf("SEND %d|%d\n",txPos, txLength);
            thread_unlockMutex(psi->txMutex);
            SendBuf(psi->hPort, pos, toWrite);
            thread_lockMutex(psi->txMutex);
            psi->txPos += psi->txLength;
            psi->txLength = 0;

            if (psi->txLength == 0 && psi->txFifoEmptyCallback != NULL) {
                thread_unlockMutex(psi->txMutex);
                psi->txFifoEmptyCallback->invoke();
                thread_lockMutex(psi->txMutex);
            }
        }
        thread_unlockMutex(psi->txMutex);
    }
}

}
