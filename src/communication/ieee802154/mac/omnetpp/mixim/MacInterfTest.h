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

#ifndef MACINTERFTEST_H_
#define MACINTERFTEST_H_

#include "MacAbstractionLayer.h"

#include <iostream>
#include <fstream>

/**This MAC layer is used for testing packet loss due
 * to interferences.
 *
 * Simulation is in "sim/mac/interf"
 *
 * Node this layer works only for OMNeT++ since
 * it requires precise synchronization among nodes
 */
class MacInterfTest: public cometos::MacAbstractionLayer {
public:
	MacInterfTest();

	virtual ~MacInterfTest() {
	}

	virtual void initialize() override;

	virtual void finish() override;

	virtual void rxEnd(cometos::Airframe *frame, node_t src, node_t dst, cometos::MacRxInfo const & info) override;

	virtual void txEnd(cometos::macTxResult_t success, cometos::MacTxInfo const& info) override;

	void traffic(cometos::Message* msg);
private:
	cometos::Message* timer;

	uint8_t i;
	uint8_t j;
	int n;

	int max_pkt;
	int max_nodes;
	int payload;

	uint8_t row;

	int counter;
	int counter2;
	std::ofstream result;
};

#endif /* MACINTERFTEST_H_ */
