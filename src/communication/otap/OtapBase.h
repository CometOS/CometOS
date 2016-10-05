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

#ifndef OTAPBASE_H_
#define OTAPBASE_H_

#include "BitVector.h"
#include "Vector.h"
#include "palFirmware.h"
#include "palFirmwareDefs.h"
#include "OtapBlockTransfer.h"
#include "ReedSolomonCoding.h"

#define OTAP_NUM_PKTS	4
#define OTAP_NUM_RED_PKTS 3
#define OTAP_PKT_SIZE 64

// to fit into a single Airframe for RemoteAccess, we use an array of
// BitVectors of size 256 (32 Byte) to keep track of successfully received
// segments
#define OTAP_BITVECTOR_SIZE 256
#define OTAP_NUM_BITVECTORS ((P_FIRMWARE_NUM_SEGS_IN_STORAGE - 1) / OTAP_BITVECTOR_SIZE + 1)


namespace cometos {

template<uint16_t LENGTH, uint16_t VEC_SIZE>
class BitVectorWrapper {
    static const uint16_t WRAPPER_BYTE_SIZE = (LENGTH - 1) / 8 + 1;
    static const uint16_t VEC_BYTE_SIZE = ((VEC_SIZE - 1) / 8 + 1);


public:
    BitVectorWrapper(bool initial_fill = false) {
        fill(initial_fill);
    }

    /**
     * Counts occurrence of a specific value.
     *
     * @param value the boolean value which should be counted
     * @return number of occurrences
     */
    uint16_t count(bool value) const {
        uint16_t counter = 0;
        for (uint16_t i = 0; i < LENGTH; i++) {
            if (get(i) == value) {
                counter++;
            }
        }
        return counter;
    }

    void fill(bool value) {
        for (uint16_t u = 0; u < OTAP_NUM_BITVECTORS; u++) {
            vectors[u].fill(value);
        }
    }

    void set(uint16_t pos, bool value) {
        if (pos >= LENGTH) {
            return;
        }
        vectors[pos / VEC_SIZE].set(pos % VEC_SIZE, value);
    }

    bool get(uint16_t pos) const {
        if (pos >= LENGTH) {
            return false; // undefined
        }
        return vectors[pos / VEC_SIZE].get(pos % VEC_SIZE);
    }

    const BitVector<VEC_SIZE>& getVector(uint16_t num) {
        return vectors[num];
    }

    uint16_t length() const {
        return LENGTH;
    }
    uint16_t getByteLenth() const {
        return WRAPPER_BYTE_SIZE;
    }
private:
    BitVector<VEC_SIZE> vectors[OTAP_NUM_BITVECTORS];
};


struct OtapInitMessage {
    OtapInitMessage(palFirmware_slotNum_t slot = 0,
                    palFirmware_segNum_t segCount = 0,
                    palFirmware_crc_t crc = 0) :
        slot(slot),
        segCount(segCount),
        crc(crc)
    {}

    palFirmware_slotNum_t slot;
    palFirmware_segNum_t segCount;
    palFirmware_crc_t crc;
};

class OtapRunMessage: public Message {
public:
    OtapRunMessage(uint8_t slot) :
        slot(slot)
    {}

    uint8_t slot;
};


class OtapBase: public OtapBlockTransfer<OTAP_NUM_PKTS, OTAP_NUM_RED_PKTS,
		OTAP_PKT_SIZE> {
public:

    static const char* const OTAP_MODULE_NAME;
    static const uint8_t NO_CURR_SLOT;

    OtapBase(const char *name);

	virtual void initialize();

	/**Runs given firmware*/
    uint8_t run(uint8_t &slot, uint16_t &delay);

    /**
     * Remotely accessible method returning a bitvector, which represents
     * part of handled firmware image.
     *
     * @param num number of the vector (starts counting at 0). That is,
     *            vector <num> stores information about segments
     *            num * OTAP_BITVECTOR_SIZE to (num + 1) * OTAP_BITVECTOR_SIZE-1
     * @return    bitvector containig information about firmware segment status
     */
    BitVector<OTAP_BITVECTOR_SIZE> getMissingVector(uint8_t & num);

    uint8_t getNumMissingVectors();

protected:
	palFirmware_slotNum_t getCurrSlot();
	void setCurrSlot(palFirmware_slotNum_t currSlot);
	palFirmware_segNum_t getSegCount();
	void setSegCount(palFirmware_segNum_t numSeg);
	BitVectorWrapper<P_FIRMWARE_NUM_SEGS_IN_STORAGE, OTAP_BITVECTOR_SIZE>& getReceivedSegments();

	palFirmware_ret_t checkInit(palFirmware_slotNum_t slot, palFirmware_segNum_t seg);

private:
	uint8_t currSlot;
    uint16_t segCount;

    void runFirmware(OtapRunMessage* msg);

    BitVectorWrapper<P_FIRMWARE_NUM_SEGS_IN_STORAGE, OTAP_BITVECTOR_SIZE> receivedSegments;
};

void serialize(ByteVector& buffer, const OtapInitMessage & value);
void unserialize(ByteVector& buffer, OtapInitMessage & value);
}

#endif /* OTAPBASE_H_ */
