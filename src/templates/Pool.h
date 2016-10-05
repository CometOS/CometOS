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

#ifndef COMETOS_POOL_H_
#define COMETOS_POOL_H_
/*INCLUDES-------------------------------------------------------------------*/

#include <stdint.h>
#include "logging.h"
#include "BitVector.h"
#include "Delegate.h"

namespace cometos {

/**
 * A pool contains a number of pre-allocated objects of a certain
 * kind and provides methods to get those items or put them back.
 *
 * This implementation is restricted to a maximum size of 255 elements.
 *
 * A class C which is to be contained within this pool has to provide
 * an implementation of the mapsTo function for the find method to be able
 * to identify the correct pre-allocated object.
 */
template<class C>
class MappedPoolBase {

public:
    MappedPoolBase(C* buffer,
                   BitVectorBase* occupied,
                   const TypedDelegate<C> &init
#ifdef OMNETPP
         , const TypedDelegate<C> &finish
#endif
         , uint8_t size):
            buffer(buffer),
            occupied(occupied),
            currSize(size),
            init(init)
#ifdef OMNETPP
            , fini(finish)
#endif
            , len(size)
    {}

    virtual ~MappedPoolBase() {};

    void initialize() {
        for (uint8_t i = 0; i < len; i++) {
            if (init.isReady()) {
                init(&buffer[i]);
            }
        }
    }

    void finish() {
#ifdef OMNETPP
        for (uint8_t i = 0; i < len; i++) {
            if (fini.isReady()) {
                fini(&buffer[i]);
            }
        }
#endif
    }

    bool isEmpty() {
        return currSize == 0;
    }

    uint8_t size() {
        return currSize;
    }

    uint8_t maxSize() {
        return len;
    }

    C* get() {
        for (uint8_t i = 0; i < len; i++) {
            if (!occupied->get(i)) {
                occupied->set(i, true);
                currSize--;
                ASSERT(checkSize());
                return &buffer[i];
            }
        }
        return NULL;
    }

    bool putBack(C* & obj) {
        for (uint8_t i = 0; i < len; i++) {
            if (obj == &buffer[i]) {
                occupied->set(i, false);
                obj = NULL;
                init(&buffer[i]);
                currSize++;
                ASSERT(checkSize());
                return true;
            }
        }
        return false;
    }

    C* find(C * other) {
        for (uint8_t i = 0; i < len; i++) {
            if (buffer + i == other) {
                return buffer + i;
            }
        }
        return NULL;
    }


    template<class K>
    C* find(K & key) {
        for (uint8_t i = 0; i < len; i++) {
            if (buffer[i].mapsTo(key)) {
                return buffer + i;
            }
        }
        return NULL;
    }

protected:
    C*& getBuffer() {;
        return buffer;
    }

    BitVectorBase*& getOccupiedVector() {
        return occupied;
    }

private:
    bool checkSize() {
        return currSize >= 0 && currSize <= len;
    }

    C* buffer;
    BitVectorBase* occupied;
    uint8_t currSize;
    TypedDelegate<C> init;
#ifdef OMNETPP
    TypedDelegate<C> fini;
#endif
    uint8_t len;
};

template<class C, uint8_t len>
class MappedPool : public MappedPoolBase<C> {
public:
    MappedPool(const TypedDelegate<C> &init
    #ifdef OMNETPP
             , const TypedDelegate<C> &finish
    #endif
             ) : MappedPoolBase<C>(theBuf,
                     occupiedVec,
                     init,
#ifdef OMNETPP
                     finish,
#endif
                     len)
    {}

private:
    C theBuf[len];
    BitVector<len> occupiedVec;
};

template<class C>
class DynMappedPool : public MappedPoolBase<C> {
public:
    DynMappedPool(const TypedDelegate<C> & init,
#ifdef OMNETPP
            const TypedDelegate<C> & finish,
#endif
            uint8_t size) : MappedPoolBase<C> (
                    new C[size],
                    new DynBitVector(size),
                    init,
#ifdef OMNETPP
                    finish,
#endif
                    size)
    {
    }

    virtual ~DynMappedPool() {
        C*& theBuf = this->getBuffer();
        delete[] theBuf;
        theBuf = nullptr;
        BitVectorBase*& occupiedVec = this->getOccupiedVector();
        delete occupiedVec;
        occupiedVec = nullptr;
    }

};

//template<class C, uint8_t len>
//class MappedPool : public Pool<C, len> {
//public:
//    MappedPool(const TypedDelegate<C> &init
//#ifdef OMNETPP
//         , const TypedDelegate<C> &finish
//#endif
//         ) : Pool<C, len>(init
//#ifdef OMNETPP
//                 , finish
//#endif
//                 ) {}
//
//
//};


} // namespace


#endif /* COMETOS_POOL_H_ */
