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

#ifndef COMETOS_BITVECTOR_H_
#define COMETOS_BITVECTOR_H_

#include <stdint.h>
#include <stdbool.h>
#include "Vector.h"

#define BITVECTOR_BYTE_LENGTH(len)	((len - 1) / 8 + 1)

/**
 * Classes to define BitVectors of arbitrary length
 *
 * @author Stefan Unterschütz, Andreas Weigel
 */

namespace cometos {

class BitVectorBase {
public:
    friend void serialize(ByteVector&, const BitVectorBase&);
    friend void unserialize(ByteVector&, BitVectorBase&);
    BitVectorBase(uint8_t* array, uint16_t len) :
                      array(array),
                      len(len)
    {}

    virtual ~BitVectorBase() {};

    void doAnd(const BitVectorBase& rhs);

    void doOr(const BitVectorBase& rhs);

    /**
     * Counts occurrence of a specific value.
     *
     * @param value the boolean value which should be counted
     * @return number of occurrences
     */
    uint16_t count(bool value) const;

    void fill(uint8_t* arr);

    void fill(bool value);

    void set(uint16_t pos, bool value);

    bool get(uint16_t pos) const;

    uint16_t length() const;

    uint16_t getByteLenth() const;

protected:
    uint8_t* getArray() {
        return array;
    }

private:
    uint8_t* array;
    uint16_t len;
};


template<uint16_t LENGTH>
class BitVector : public BitVectorBase {
public:

	BitVector(bool initial_fill = false) :
	    BitVectorBase(array, LENGTH)
	{
		fill(initial_fill);
	}

private:
	uint8_t array[BITVECTOR_BYTE_LENGTH(LENGTH)];
};

class DynBitVector : public BitVectorBase {
public:
    DynBitVector(uint8_t len, bool initial_fill = false) :
        BitVectorBase(new uint8_t[BITVECTOR_BYTE_LENGTH(len)], len)
    {
        fill(initial_fill);
    }

    virtual ~DynBitVector() {
        delete[] this->getArray();
    }

private:
    // declare assignment operator and copy constructor private to prevent them
    DynBitVector& operator=(const DynBitVector& rhs);
    DynBitVector(const DynBitVector& rhs);
};

template<uint16_t LENGTH>
BitVector<LENGTH> operator|(const BitVector<LENGTH> &op1, const BitVector<LENGTH> &op2) {
	BitVector<LENGTH> res = op1;
	res.doOr(op2);
	return res;
}

template<uint16_t LENGTH>
BitVector<LENGTH> operator&(const BitVector<LENGTH> &op1, const BitVector<LENGTH> &op2) {
	BitVector<LENGTH> res = op1;
	res.doAnd(op2);
	return res;
}


void serialize(ByteVector& buffer, const BitVectorBase& value);
void unserialize(ByteVector& buffer, BitVectorBase& value);

}

#endif /* BITVECTOR_H_ */
