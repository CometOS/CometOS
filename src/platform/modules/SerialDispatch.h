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
 * @author Andreas Weigel
 */

#ifndef SERIAL_DISPATCH_H_
#define SERIAL_DISPATCH_H_

/*INCLUDES-------------------------------------------------------------------*/

#include "LowerEndpoint.h"
#include "SerialComm.h"
#include "cometosError.h"
#include <map>


namespace cometos {

class SerialDispatch;

class SerialOutput {
public:
    SerialOutput(const char* port,
                 uint16_t id,
                 SerialDispatch* ptr,
                 uint32_t baudrate,
                 uint16_t frameTimeout);

    ~SerialOutput();

    void sendRequest(DataRequest* msg);

    bool initializationSuccessful();

private:
    OutputGate<DataRequest>* gateOut;
    InputGate<DataIndication>* gateIn;
    SerialComm* sc;

private:
    SerialOutput(const SerialOutput& rhs);
    SerialOutput& operator=(const SerialOutput& rhs);
};


/**
 * Base class meant to be subclassed by a python class that configures
 * the forwarding to a number of SerialComm instances. This class then takes
 * care of the actual forwarding. The provided hook for a subclass is called
 * AFTER the initialization phase.
 *
 *           +----------------+
 *           |   Upper Layer  |
 *           +----------------+
 *                   |
 *           +----------------+
 *           | SerialDispatch |
 *           +----------------+
 *             /     |      \
 *            /      |       \
 *           /       |        \
 *     +-----+    +-----+    +-----+
 *     | SC1 |    | SC2 |    | SC3 |
 *     +-----+    +-----+    +-----+
 *
 *
 */
class SerialDispatch: public cometos::LowerEndpoint {
public:

    static const char* const DEFAULT_MODULE_NAME;

	SerialDispatch();
	virtual ~SerialDispatch();

	virtual void initialize();

	virtual void handleRequest(DataRequest* msg);

	virtual void handleIndication(DataIndication* msg);

	virtual void doInitializeGates() = 0;

	cometos_error_t createForwarding(const char* port,
	                     node_t address,
	                     uint32_t baudrate,
	                     uint16_t frameTimeout);

private:
	void initializeGates(cometos::Message* msg);

	static uint16_t portNumber;
	std::map<node_t, SerialOutput*> forwarderMap;
};

} /* namespace cometos */
#endif /* SERIAL_DISPATCH_H_ */
