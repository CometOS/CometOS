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

#ifndef REQUESTRESPONSE_H_
#define REQUESTRESPONSE_H_

#include <cometos.h>

namespace cometos {

class RequestId {
};

class Response: public Message {
public:
	RequestId *getRequestId() {
		return id;
	}

	Response() :
			id(NULL) {
	}

	virtual ~Response() {
		if (id != NULL) {
			delete id;
		}
	}

	void setResponseId(RequestId *id) {
		if (this->id != NULL) {
			delete this->id;
		}
		this->id = id;
	}
private:
	RequestId *id;
};

template<class R>
class Request: public Message {
public:

	/**Optionally, a user defined RequestId can be passed. However, this request id
	 * must be dynamically allocated and will be managed by this object.
	 *
	 */
	Request(const TypedDelegate<R> &delegate, RequestId *rid = NULL) :
			id(NULL), delegate(delegate) {
		if (rid != NULL) {
			id = rid;
		}
		if (this->delegate.isReady() && rid == NULL) {
			// allocation is used to get unique pointer (and thus a unique id)
			id = new RequestId;
		}
	}

	virtual ~Request() {
		// no confirm was send!!!
		if (id != NULL) {
			delete id;
		}
	}

	const TypedDelegate<R>* getResponseDelegate() {
	    return &(this->delegate);
	}

	void setResponseDelegate(const TypedDelegate<R> &delegate) {
	    this->delegate=delegate;
	}

    // send response message
	void response(R* response) {
		ASSERT(response!=NULL);

		if (!delegate.isReady()) {
			delete response;
			return;
		}
		response->setResponseId(id);
		id = NULL;
		delegate(response);
	}

	RequestId *getRequestId() {
		return id;
	}

protected:
	RequestId *id;

	TypedDelegate<R> delegate;
}
;

}

#endif /* REQUESTRESPONSE_H_ */
