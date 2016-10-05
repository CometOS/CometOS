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
 * @author Stefan Unterschuetz, Andreas Weigel
 */

#include "OutputStream.h"


#include "palId.h"
#include "NetworkTime.h"
#include "palExec.h"

#ifdef PAL_EXEC_UTIL
#include "palExecUtils.h"
#endif

#ifdef PAL_LED
#include "palLed.h"
#endif

#ifdef PAL_PERS
#include "palPers.h"
#endif

#include "SystemMonitor.h"
#include "memory.h"
#include "DataRequest.h"
#include "DataResponse.h"
#include "AirString.h"
#include "mac_definitions.h"

#ifndef BASESTATION_ADDR
#define BASESTATION_ADDR 0
#endif

extern uint8_t * cometos_resetStatus;

Define_Module(cometos::SystemMonitor);

namespace cometos {

const char * const SystemMonitor::DEFAULT_MODULE_NAME = "sys";

void serialize(ByteVector& buffer, const CoreUtilization& value) {
    serialize(buffer, value.cpu);
    serialize(buffer, value.mem);
}

void unserialize(ByteVector& buffer, CoreUtilization& value) {
    unserialize(buffer, value.mem);
    unserialize(buffer, value.cpu);
}

void serialize(ByteVector& buffer, const AssertShortInfo& value) {
    serialize(buffer, value.fileId);
    serialize(buffer, value.line);
    serialize(buffer, value.reset);
    serialize(buffer, value.resetOffset);
    serialize(buffer, value.pc);
    serialize(buffer, value.fwVersion);
}

void unserialize(ByteVector& buffer, AssertShortInfo& value) {
    unserialize(buffer, value.fwVersion);
    unserialize(buffer, value.pc);
    unserialize(buffer, value.resetOffset);
    unserialize(buffer, value.reset);
    unserialize(buffer, value.line);
    unserialize(buffer, value.fileId);
}

SystemMonitor::SystemMonitor(const char* service_name,
                             RemoteAccess* ra) :
        RemoteModule(service_name),
        pingValue(0),
        heartbeatInterval(0),
        heartbeat(this, "hb"),
        bootAfterReset(this, "bar"),
        asyncTest(this, "taD"),
        ra(ra)
{

}


CoreUtilization SystemMonitor::getUtilization() {
    CoreUtilization val;

#ifndef OMNETPP
#ifndef SCHEDULER_DISABLE_MONITORING
    val.cpu = cometos::getScheduler().getUtil();
#endif
    val.mem = heapGetUtilization();
#endif
    return val;
}


void SystemMonitor::initialize() {
    remoteDeclare(&SystemMonitor::ping, "ping");
    remoteDeclare(&SystemMonitor::getUtilization, "util");
    remoteDeclare(&SystemMonitor::toggleLed, "led");
    remoteDeclare(&SystemMonitor::runHeartbeat, "run");
    remoteDeclare(&SystemMonitor::getTimestamp, "ts");
    remoteDeclare(&SystemMonitor::assertRead, "ar"); // TODO deprecate
    remoteDeclare(&SystemMonitor::getAssertInfo, "ai");
    remoteDeclare(&SystemMonitor::assertClear, "ac");
    remoteDeclare(&SystemMonitor::assertTest, "at");
    remoteDeclare(&SystemMonitor::testAsync, "ta", "taD");
    remoteDeclare(&SystemMonitor::getFirmwareVersion, "fwv");

    schedule(new Message, &SystemMonitor::bootEvent, 0);

#ifdef DEBUG_MESSAGE_ALLOCATION
    remoteDeclare(&SystemMonitor::msgAlloc, "ma");
#endif
}

void SystemMonitor::toggleLed(uint8_t &val) {
#ifdef PAL_LED
    palLed_toggle(val);
#endif
}

void SystemMonitor::runHeartbeat(uint16_t &heartbeatInterval) {
    if (heartbeatInterval == 0) {
        return;
    }

    this->heartbeatInterval = heartbeatInterval;
    schedule(new Message, &SystemMonitor::timeoutHeartbeat,
            this->heartbeatInterval);
}

time_ms_t SystemMonitor::getTimestamp() {
    return NetworkTime::get();
}

void SystemMonitor::timeoutHeartbeat(Message* msg) {
    node_t id = 0;
#ifdef PAL_ID
    id= palId_id();
#else
    id = MAC_BROADCAST;
#endif
    heartbeat.raiseEvent(id);
    schedule(msg, &SystemMonitor::timeoutHeartbeat, this->heartbeatInterval);

}

uint16_t SystemMonitor::ping() {
   return pingValue++;
}

AssertShortInfo SystemMonitor::getAssertInfo() {
#ifdef PAL_EXEC_UTIL
    uint16_t line = 0;
    uint16_t fileId = 0;
    palExec_readAssertInfoShort(line, fileId);
    AssertShortInfo asi(line, fileId);
#else
    AssertShortInfo asi(0,0);
#endif
    return asi;
}

firmwareVersion_t SystemMonitor::getFirmwareVersion() {
    firmwareVersion_t v = 0;
#if defined PAL_FIRMWARE or defined PAL_FIRMWARE_ASYNC
    v = firmware_getVersion();
#endif
    return v;
}

void SystemMonitor::assertRead() {
#ifdef PAL_EXEC_UTIL
    uint16_t line = 0;
    uint16_t fileId = 0;
    if (palExec_readAssertInfoShort(line, fileId)) {
        cometos::getCout()<<"ASSERT: FileId=" << fileId <<" Line=" << line << cometos::endl;
    } else {
        char file[AirString::MAX_LEN];
        if (palExec_readAssertInfoLong(line, file, AirString::MAX_LEN)) {
            AirString fileStr(file);
            cometos::getCout() << "ASSERT: File=" << fileStr.getStr() << ":" << line << cometos::endl;
        } else {
            cometos::getCout() << "ASSERT info empty" << cometos::endl;
        }
    }
#endif
}

void SystemMonitor::testAsync(timeOffset_t & delay) {
    schedule(new Message, &SystemMonitor::asyncEvent, delay);
}

void SystemMonitor::asyncEvent(Message * msg) {
    delete (msg);
    asyncTest.raiseEvent(pingValue);
    pingValue++;
}

void SystemMonitor::assertClear() {
#ifdef PAL_EXEC_UTIL
    palExec_clearAssertInfo();
#endif
}

void SystemMonitor::assertTask(Message * msg) {
    delete (msg);
    ASSERT(false);
}

void SystemMonitor::assertTest(AssertShortInfo & cfg) {
    schedule(new Message(), &SystemMonitor::assertTask, cfg.resetOffset);
}

void SystemMonitor::bootEvent(Message * msg) {
    delete (msg);
    if (ra != NULL) {
		bootAfterReset.subscribe(ra, BASESTATION_ADDR, 1);
		AssertShortInfo asi = getAssertInfo();
#ifdef PAL_EXEC_UTIL
		asi.pc = palExec_getPcAtReset();
		asi.reset = palExec_getResetReason();
#endif
#if defined PAL_FIRMWARE or defined PAL_FIRMWARE_ASYNC
		asi.fwVersion = getFirmwareVersion();
#endif
		bootAfterReset.raiseEvent(asi);
    }  
}

#ifdef DEBUG_MESSAGE_ALLOCATION
void SystemMonitor::msgAlloc() {
    Message::printAllocationCount();
    Message::printOwners();
}
#endif

}
/* namespace cometos */
