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

#ifndef OTAP_H_
#define OTAP_H_

#include "OtapBase.h"


namespace cometos {

/**An easy OTAP protocol
 */
class Otap: public OtapBase {
public:


	virtual ~Otap();
	Otap();

	virtual void recvSegment(uint8_t * data, uint16_t segId);

	void initialize();

	/**Prepare a slot for a new firmware.
	 *
	 * @param slot	slot for new firmware
	 * @param size	size of new firmware (number of segments)
	 */
	uint8_t initiate(OtapInitMessage & msg);


	/**Verify current firmware and store version id for slot.
	 * A verify is applied for the current active slot, for
	 * which initiate is called.
	 * */
	uint8_t verify(uint16_t &crc, uint32_t &address_offset);



private:



	/*
	 virtual Packet* command(const char *argv[], const uint8_t argc);

	 void receive(Packet *msg);

	 void prepare(uint16_t version);

	 void initialize();

	 void timeout(Message *msg);

	 public:
	 OtapApp(const char *name = NULL);

	 private:

	 ReedSolomonCoding<OTAP_PKT_SEG, OTAP_RED_SEG> coding;
	 uint8_t *pkt[OTAP_PKT_SEG];
	 uint8_t pktArray[OTAP_PKT_SEG * OTAP_PKT_SIZE]; // actually this contains the whole segment
	 uint8_t *red[OTAP_RED_SEG];
	 uint8_t redArray[OTAP_RED_SEG * OTAP_PKT_SIZE];

	 BitVector<OTAP_MAX_NUM_SEGS> recvSeg;
	 BitVector<OTAP_PKT_SEG> recvPkt;
	 BitVector<OTAP_RED_SEG> recvRed;

	 uint16_t curVers;
	 uint8_t curSeg;

	 Message *timer;
	 */
};

}

#endif /* OTAP_H_ */
