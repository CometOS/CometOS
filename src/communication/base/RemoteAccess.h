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
 * @author Stefan Unterschütz
 */

#ifndef REMOTEACCESS_H_
#define REMOTEACCESS_H_

/*INCLUDES-------------------------------------------------------------------*/

#include <cometos.h>
#include "DataRequest.h"
#include "DataResponse.h"
#include "DataIndication.h"
#include "Endpoint.h"

/*PROTOTYPES-----------------------------------------------------------------*/

namespace cometos {

enum RemoteAccessStatus {
    RA_SUCCESS = 0,
    RA_ERROR_NO_SUCH_MODULE = 1,
    RA_ERROR_NO_SUCH_VARIABLE = 2,
    RA_ERROR_NO_SUCH_METHOD = 3,
    RA_ERROR_NO_SUCH_EVENT = 4,
    RA_ERROR_INVALID_METHOD_TYPE = 5
};

enum RemoteAccessType {
    RA_REMOTE_VARIABLE =0,
    RA_REMOTE_METHOD = 1,
    RA_REMOTE_EVENT_SUBSCRIBE = 2,
    RA_REMOTE_EVENT_UNSUBSCRIBE = 3,
    RA_REMOTE_METHOD_EVENT = 4,
    RA_REMOTE_EVENT = 255
};


/**
 * Base class for communication layer. DataRequest and DataIndication
 * are used as exchange format.
 *
 * Packet format for requests with the current, "stack"-based approach:
 *             <--- read in this direction
 * +--------+------+------+--------+-----+
 * | params | name | type | module | seq |
 * +--------+------+------+--------+-----+
 * |  var   |  var |   1  |   var  |  1  |
 * +--------+------+------+--------+-----+
 *
 * seq     is the sequence number of the message
 * module  is an AirString containing the module name after a 1 byte len field
 * type    is the type of the remote command (see enum)
 * name    AirString with a 1 byte length field and the name of the
 *         variable/event/method to read/subscribe/call
 *
 *
 * Response format is
 * +--------+--------+------+
 * |  ret   | status |  seq |
 * +--------+--------+------+
 * |  var   |    1   |   1  |
 * +--------+--------+------+
 *
 * ret    Return value, variable depending on method/var/event type
 * status Status code
 *
 */
class RemoteAccess: public Endpoint {
public:
    static const char * const DEFAULT_MODULE_NAME;

	RemoteAccess(const char* service_name = DEFAULT_MODULE_NAME);

	virtual void handleIndication(DataIndication* msg);

	void initialize();

	void raise(Airframe* frame, node_t dst);
private:
};

} /* namespace cometos */

#endif /* REMOTEACCESS_H_ */
