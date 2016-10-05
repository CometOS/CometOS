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

#ifndef COUNTERBASEDFLOODING_H_
#define COUNTERBASEDFLOODING_H_

#include "Layer.h"
#include "Queue.h"
#include "NwkHeader.h"


#define CB_FLOODING_HISTORY_SIZE 	5

typedef struct {
    cometos::NwkHeader header;
	uint8_t count;
} flooding_entry_t;

bool operator==(const flooding_entry_t& a, const cometos::NwkHeader& b);

class CounterBasedFlooding: public cometos::Layer {
public:

	virtual void initialize();
	virtual void finish();


	virtual void handleRequest(cometos::DataRequest* msg);
	virtual void handleIndication(cometos::DataIndication* msg);

	/**
	 * Checks if header is in queue and increments counter value
	 */
	bool checkAndIncrement(cometos::NwkHeader& header);


	void forward(cometos::DataRequest *req);

private:
	uint8_t sequence;
	uint8_t maxCounter;
	uint16_t sendingOffset;
	uint16_t forwardedStats;

	cometos::Queue<flooding_entry_t, CB_FLOODING_HISTORY_SIZE> history;

};

#endif /* COUNTERBASEDFLOODING_H_ */
