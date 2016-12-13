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

#ifndef AIRFRAME_H_
#define AIRFRAME_H_

/*INCLUDES-------------------------------------------------------------------*/

#include "cometos.h"
#include "Vector.h"
#include "primitives.h"
#include "templates.h"
#include "OutputStream.h"
#include "logging.h"
/*TYPES----------------------------------------------------------------------*/


/*MACROS---------------------------------------------------------------------*/

/**Maximal size of data
 */

#ifdef PAL_MAC
#include "mac_definitions.h"
#define AIRFRAME_MAX_SIZE MAC_PACKET_BUFFER_SIZE
#else
#define AIRFRAME_MAX_SIZE 120
#endif

namespace cometos {

/*TYPES----------------------------------------------------------------------*/
typedef uint8_t pktSize_t;

/**Managed byte array for packets*/
class AirframeData: public ByteVector {
public:
	AirframeData()
	: ByteVector(buffer, AIRFRAME_MAX_SIZE) {
	}

	void printFrame(OutputStream* outputStream = NULL, bool prefix = false) const {
        if(outputStream == NULL) {
            outputStream = &cometos::getCout();
        }
#ifdef ENABLE_LOGGING
        if(prefix) {
            (*outputStream) << PREFIX;
        }
#endif
        (*outputStream) << cometos::dec;
        (*outputStream) << "len= " << (uint16_t)getSize();
        (*outputStream) << cometos::hex;
        for (uint16_t i = 0; i < getSize(); i++) {
            if (i % 16 == 0) {
                (*outputStream) << cometos::endl;
#ifdef ENABLE_LOGGING
                if(prefix) {
                    (*outputStream) << PREFIX;
                }
#endif
                (*outputStream) << " ";
            }
            (*outputStream) << "0x";
            uint8_t d = buffer[i];
            if(d <= 0xF) {
                (*outputStream) << "0";
            }
            (*outputStream) << (uint16_t)d << " ";
        }
        (*outputStream) << cometos::dec << cometos::endl;
#if 0
        (*outputStream) << "xxxxxxxxxxxxxx" << cometos::endl;
        for (uint16_t i = 0; i < getLength(); i++) {
            char a[2];
            a[0] = data[i];
            a[1] = 0;
            (*outputStream) << a << " ";
        }
        (*outputStream) << cometos::endl;
        (*outputStream) << "yyyyyyyyyyyyy" << cometos::endl;
#endif
        }

private:
	uint8_t buffer[AIRFRAME_MAX_SIZE];
};


/*PROTOTYPES-----------------------------------------------------------------*/

/**Encapsulates Airframes/Over The Air packets. Allows de/serialization to a
 * managed byte array. The array is used as a stack (last in, first out).
 * The shift operator can be applied to put data into the Airframe. For
 * structs this serialization and deserialization process should be explicitly
 * be given instead of invoking encapsulate and decapsulate in order to
 * provide interoperability.
 */
class Airframe: public ObjectContainer {
	friend class MacAbstractionLayer;
public:

	ByteVector& getArray() {
		return data;
	}

	inline pktSize_t getLength() const {
		return data.getSize();
	}

	inline pktSize_t getMaxLength() const {
		return data.getMaxSize();
	}

	inline void clear() {
		data.clear();
	}

	inline uint8_t *getData() {
		return data.getBuffer();
	}

    inline const uint8_t *getData() const {
        return data.getConstBuffer();
    }

	inline uint8_t getByte(pktSize_t position) const {
		return data[position];
	}

	/**Fills data with dummy data until*/
	inline void setLength(pktSize_t size) {
		data.setSize(size);
	}

	/**Note that this doesn't copy Meta Data !*/
	Airframe* getCopy() const;

	/**Copies also MetaData*/
	Airframe* getDeepCopy() const;

	Airframe& operator=(const Airframe& other);

	void printFrame(OutputStream* outputStream = NULL, bool prefix = false) const {
	    data.printFrame(outputStream, prefix);
	}

	template<class C>
	Airframe &operator>>(C& val) {
		unserialize(data, val);
		return *this;
	}

	template<class C>
	Airframe &operator<<(const C& val) {
		serialize(data, val);
		return *this;
	}

	uint8_t popFront() {
        return data.popFront();
	}

	void pushFront(uint8_t element) {
        data.pushFront(element);
	}

private:
	AirframeData data;
};

} /* namespace cometos */

#endif /* AIRFRAME_H_ */
