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
#ifndef SYSTEMMONITOR_H_
#define SYSTEMMONITOR_H_

#include "RemoteModule.h"
#include "Serializable.h"
#include "firmwareVersion.h"
#include "primitives.h"
#include "RemoteAccess.h"

namespace cometos {

class CoreUtilization {
public:
	CoreUtilization() :
			cpu(0), mem(0) {
	}
	uint8_t cpu;
	uint8_t mem;
};

void serialize(ByteVector& buffer, const CoreUtilization& value);
void unserialize(ByteVector& buffer, CoreUtilization& value);

class AssertShortInfo {
public:
    AssertShortInfo() :
        line(0), fileId(0), reset(false), resetOffset(0), pc(0), fwVersion(0)
    {}

    AssertShortInfo(uint16_t line, uint16_t fileId) :
        line(line), fileId(fileId), reset(false), resetOffset(0), pc(0), fwVersion(0)
    {}

    uint16_t line;
    uint16_t fileId;
    uint8_t reset;
    timeOffset_t resetOffset;
    uint32_t pc;
    firmwareVersion_t fwVersion;
};

void serialize(ByteVector& buffer, const AssertShortInfo& value);
void unserialize(ByteVector& buffer, AssertShortInfo& value);

/**
 * Monitoring module for CometOS. Following functionalities
 * are provided:
 * <li> lists loaded modules
 * <li> memory allocation
 * <li> lists messages and message owners
 * <li> CPU utilization
 * <li> provides platform dependent services (e.g. returns identifier)
 */
class SystemMonitor: public RemoteModule {
public:

    static const char * const DEFAULT_MODULE_NAME;

	SystemMonitor(const char* service_name = DEFAULT_MODULE_NAME,
	              RemoteAccess* ra = NULL);

	void initialize();

	uint16_t ping();
	uint16_t pingValue;

	CoreUtilization getUtilization();

	void toggleLed(uint8_t &val);


	void runHeartbeat(uint16_t &heartbeatInterval);

	time_ms_t getTimestamp();

	AssertShortInfo getAssertInfo();

	void assertRead();

	void assertClear();

	void assertTask(Message * msg);

	void assertTest(AssertShortInfo & cfg);

	void testAsync(timeOffset_t & delay);

	void asyncEvent(Message * msg);

	void bootEvent(Message * msg);

	void timeoutHeartbeat(Message* msg);

	firmwareVersion_t getFirmwareVersion();

#ifdef DEBUG_MESSAGE_ALLOCATION
	void msgAlloc();
#endif
	uint16_t heartbeatInterval;
	RemoteEvent<uint16_t> heartbeat;
	RemoteEvent<AssertShortInfo> bootAfterReset;

	RemoteEvent<uint16_t> asyncTest;
	RemoteAccess* ra;
};

} /* namespace cometos */
#endif /* SYSTEMMONITOR_H_ */
