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
#include "Task.h"
#include "OutputStream.h"
#include "palExecUtils.h"
#include "palGPIOPin.h"
#include "BroadcastBeacon.h"
#include "palLocalTime.h"
#include <avr/wdt.h>
#include "Morse.h"

#include "palPin.h"

#include "CC110lApi.h"

#include "palSerial.h"

using namespace cometos;

#ifdef BOARD_AutoRNode
#define SENDER 0
GPIOPin<IOPort::E,7> gdo0pin;
GPIOPin<IOPort::E,3> chipSelect;
GPIOPin<IOPort::B,3> miso;
#else
#ifdef BOARD_lorasender
#define SENDER 1
GPIOPin<IOPort::D,2> gdo0pin;
GPIOPin<IOPort::B,2> chipSelect;
GPIOPin<IOPort::B,4> miso;
#endif
#endif

void setup();

CC110lApi sender(&gdo0pin,&chipSelect,&miso,CALLBACK_FUN(setup));

uint16_t packetLength = 6;

void prompt(cometos_error_t result) {
    if(result == COMETOS_SUCCESS) {
    	cometos::getCout() << "> ";
    }
    else {
    	cometos::getCout() << "% ";
    }
}

uint8_t sendData[] = "jfds_M(W#d*BM$WJdSDMV NV)(*#R%MVWSFJLV)($*M^TQV(<#R";

time_ms_t sendStart = 0;
time_ms_t sendEnd = 0;

uint16_t packetsToSend = 0;
uint16_t packetCount = 0;

void send();
void delayFinished();
SimpleTask delayTask(delayFinished);
void delayFinished() {
    palExec_atomicBegin();
    if(packetsToSend) {
        send();
    }
    else {
        prompt(COMETOS_SUCCESS);
    }
    palExec_atomicEnd();
}

void txReady() {
    palExec_atomicBegin();
    sendEnd = palLocalTime_get();
    cometos::getScheduler().add(delayTask,(sendEnd-sendStart)*9+1);
    palExec_atomicEnd();
}

void send() {
#if 1 
//SENDER
    palExec_atomicBegin();
    if(packetsToSend) {
        packetsToSend--;
        sender.send(packetLength,sendData,CALLBACK_FUN(txReady));
        sendStart = palLocalTime_get();
    }
    else {
        prompt(COMETOS_SUCCESS);
    }
    palExec_atomicEnd();
#else
    cometos::getCout() << "This is not a sender!" << cometos::endl;
    prompt(COMETOS_ERROR_INVALID);
#endif
}

bool packetPending = false;
bool packetOk = false;
int16_t lastRSSI = -200;

void receive(uint8_t length, uint8_t* data, int16_t rssi, uint8_t crc_ok) {
    sender.receivePacket(CALLBACK_FUN(receive));

    palExec_atomicBegin();
    packetOk = true;
    for(int i = 0; i < length; i++) {
      if(data[i] != sendData[i]) {
        packetOk = false;
      }
    }

    if(packetOk) {
        packetCount++;
    }

    lastRSSI = rssi;
    packetPending = true;
    palExec_atomicEnd();
}

#define ABS(x) (((x)<0)?(-(x)):(x))

void request() {
#if !SENDER
    palExec_atomicBegin();
    if(packetPending) {
        cometos::getCout() << "y";
        if(packetOk) {
                cometos::getCout() << "s";
        }
        else {
                cometos::getCout() << "f";
        }

        cometos::getCout() << " " << lastRSSI/10 << "." << ABS(lastRSSI%10) << cometos::endl;
    }
    else {
        cometos::getCout() << "n" << cometos::endl;
    }
    packetPending = false;
    palExec_atomicEnd();
#else
    cometos::getCout() << "This is not a receiver!" << cometos::endl;
    prompt(COMETOS_ERROR_INVALID);
#endif
}

void multi_request() {
#if !SENDER
    palExec_atomicBegin();
    cometos::getCout() << dec << packetCount << cometos::endl;
    sender.printLastReceptionParameters();
    packetCount = 0;
    palExec_atomicEnd();
#else
    cometos::getCout() << "This is not a receiver!" << cometos::endl;
    prompt(COMETOS_ERROR_INVALID);
#endif
}

////////////////////////////////////////////////////

PalSerial* serial;

void rxSerial();
SimpleTask rxSerialTask(rxSerial);
void rxSerialFull();
SimpleTask rxSerialFullTask(rxSerialFull);

void rxSerial() {
    cometos::getScheduler().add(rxSerialFullTask,40);
}

Morse morse;
void dit();
SimpleTask ditTask(dit);
void dit() {
    if(morse.nextDit()) {
        sender.sendStatic();
    }
    else {
        sender.enterIdle();
    }

    if(morse.ditsLeft() > 0) {
        getScheduler().replace(ditTask,MS_PER_DIT(20));
    }
    else {
        sender.enterIdle();
        prompt(COMETOS_SUCCESS);
    }
}

#define COMMAND_MAX_ARGUMENTS 6
#define COMMAND_DELIMITERS      " "
#define MAX_CMD_LENGTH 100

char serialData[MAX_CMD_LENGTH+1];

void handleCommand(char *argv[], uint8_t argc) {
    bool result = COMETOS_SUCCESS;

    // ping command
    if (argc == 1 && 0 == strcmp("ping", argv[0])) {
        getCout() << "pong" << endl;
    } else if  (0 == strcmp("reset", argv[0])) {
        wdt_enable(WDTO_30MS); while(1) {};
    } else if  (0 == strcmp("type", argv[0])) {
#if SENDER
        cometos::getCout() << "sender" << cometos::endl;
#else
        cometos::getCout() << "receiver" << cometos::endl;
#endif
    } else if  (argc == 1 && 0 == strcmp("send", argv[0])) {
        packetsToSend = 1;
        send();
        return; // do not send promt yet
    } else if  (argc == 2 && 0 == strcmp("send", argv[0])) {
        sender.setUnamplifiedOutputPower(atoi(argv[1]));
        packetsToSend = 1;
        send();
        return; // do not send promt yet
    } else if  (argc == 3 && 0 == strcmp("msend", argv[0])) {
        sender.setUnamplifiedOutputPower(atoi(argv[2]));
        packetsToSend = atol(argv[1]);
        send();
        return; // do not send promt yet
    } else if  (0 == strcmp("request", argv[0])) {
        request();
    } else if  (0 == strcmp("mrequest", argv[0])) {
        multi_request();
    } else if  (argc >= 2 && 0 == strcmp("morse", argv[0])) {
        // merge commands
        for(uint8_t i = 1; i < argc-1; i++) {
            argv[i][strlen(argv[i])] = ' ';
        }

        morse.initMorse(argv[1],strlen(argv[1]));
        getScheduler().replace(ditTask,0);
        return; // do not send promt yet
    } else if  (argc == 2 && 0 == strcmp("pwr", argv[0])) {
        result = sender.setUnamplifiedOutputPower(atoi(argv[1]));
    } else if  (argc == 2 && 0 == strcmp("rate", argv[0])) {
        result = sender.setDataRate(atol(argv[1]));
    } else if  (argc == 2 && 0 == strcmp("mod", argv[0])) {
        if(0 == strcmp("2-FSK",argv[1])) {
            result = sender.setModulation(CC110lApi::Modulation::FSK2);
        }
        else if(0 == strcmp("GFSK",argv[1])) {
            result = sender.setModulation(CC110lApi::Modulation::GFSK);
        }
        else if(0 == strcmp("OOK",argv[1])) {
            result = sender.setModulation(CC110lApi::Modulation::OOK);
        }
        else {
            cometos::getCout() << "Unknown modulation" << cometos::endl;
            result = COMETOS_ERROR_INVALID;
        }
    } else if  (argc == 2 && 0 == strcmp("coding", argv[0])) {
        if(0 == strcmp("uncoded",argv[1])) {
            result = sender.setCoding(CC110lApi::Coding::UNCODED);
        }
        else if(0 == strcmp("manch",argv[1])) {
            result = sender.setCoding(CC110lApi::Coding::MANCHESTER);
        }
        else {
            cometos::getCout() << "Unknown coding" << cometos::endl;
            result = COMETOS_ERROR_INVALID;
        }
    } else if  (argc == 2 && 0 == strcmp("sync", argv[0])) {
        if(0 == strcmp("sync_15_16",argv[1])) {
            result = sender.setSync(CC110lApi::Sync::SYNC_15_16);
        }
        else if(0 == strcmp("sync_16_16",argv[1])) {
            result = sender.setSync(CC110lApi::Sync::SYNC_16_16);
        }
        else if(0 == strcmp("sync_30_32",argv[1])) {
            result = sender.setSync(CC110lApi::Sync::SYNC_30_32);
        }
        else {
            cometos::getCout() << "Unknown" << cometos::endl;
            result = COMETOS_ERROR_INVALID;
        }
    } else if  (argc == 2 && 0 == strcmp("offset", argv[0])) {
        result = sender.setFrequencyOffset(atoi(argv[1]));
    } else if  (argc == 2 && 0 == strcmp("preamble", argv[0])) {
        result = sender.setPreamble(atoi(argv[1]));
    } else if  (argc == 2 && 0 == strcmp("cbw", argv[0])) {
        result = sender.setChannelBandwidth(atol(argv[1]));
    } else if  (argc == 2 && 0 == strcmp("dev", argv[0])) {
        result = sender.setDeviation(atol(argv[1]));
    } else if  (argc == 2 && 0 == strcmp("chan", argv[0])) {
        result = sender.setChannel(atoi(argv[1]));
    } else if  (argc == 2 && 0 == strcmp("rxatt", argv[0])) {
        result = sender.setRXAttenuation(atoi(argv[1]));
    } else if  (argc == 2 && 0 == strcmp("hg", argv[0])) {
        if(0 == strcmp("on", argv[1])) {
            sender.getCC1190().highGainEnable();
        }
        else {
            sender.getCC1190().highGainDisable();
        }
        result = COMETOS_SUCCESS;
    } else if  (argc == 2 && 0 == strcmp("amp", argv[0])) {
        if(0 == strcmp("on", argv[1])) {
            sender.getCC1190().enable();
        }
        else {
            sender.getCC1190().disable();
        }
        result = COMETOS_SUCCESS;
    } else if  (argc == 2 && 0 == strcmp("static", argv[0])) {
        if(0 == strcmp("on", argv[1])) {
            sender.sendStatic();
        }
        else {
            sender.enterIdle();
        }
        result = COMETOS_SUCCESS;
    } else if  (argc == 1 && 0 == strcmp("dump", argv[0])) {
	    sender.dumpSettings();
        result = COMETOS_SUCCESS;
    } else if  (argc == 2 && 0 == strcmp("payload", argv[0])) {
        uint16_t size = atoi(argv[1]);
        if(size >= sizeof(sendData)) {
            cometos::getCout() << "Too large" << cometos::endl;
            result = COMETOS_ERROR_INVALID;
        }
        else {
            cometos::getCout() << "len " << dec << (uint16_t)size << cometos::endl;
            packetLength = size;
            result = COMETOS_SUCCESS;
        }
    } else if  (0 == strcmp("mode", argv[0])) {
        cometos::getCout() << "Mode: " << dec << (uint16_t)sender.getMode() << cometos::endl;
    } else {
        cometos::getCout() << "Unknown" << cometos::endl;
        result = COMETOS_ERROR_INVALID;
    }

    prompt(result);
}

void splitCommand(char *cmd) {
    uint8_t argc = 0;
    char *argv[COMMAND_MAX_ARGUMENTS] = { 0 };

    argv[argc] = strtok(cmd, COMMAND_DELIMITERS);

    if (argv[argc] == NULL) {
        return;
    }

    while (argv[argc] != NULL || (argc + 1) == COMMAND_MAX_ARGUMENTS) {
        argc++;
        argv[argc] = strtok(NULL, COMMAND_DELIMITERS);
    }
    handleCommand(argv, argc);
}

void rxSerialFull() {
    uint16_t len = serial->read((uint8_t*)serialData,sizeof(serialData));
    serialData[len-1] = '\0'; // remove line feed

    getCout() << serialData << endl;

    splitCommand(serialData);
}

void setup() {
    sender.enableAmp();
    sender.setUnamplifiedOutputPower(-30);
}

void start() {
    sender.setup();
    sender.receivePacket(CALLBACK_FUN(receive));

    serial = PalSerial::getInstance(0);  
    serial->init(57600,&rxSerialTask,NULL,NULL);
    prompt(COMETOS_SUCCESS);
}
SimpleTask startTask(start);

int main() {
    palLed_init();
    cometos::initialize();
    getCout() << "Booted" << endl; // this also initializes the output stream!

    cometos::getScheduler().add(startTask,1000);

    cometos::run();
    return 0;
}

