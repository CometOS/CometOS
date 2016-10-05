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

#ifndef POINTER_H_
#define POINTER_H_

/**
 * @author Stefan Unterschuetz
 */

namespace cometos {

template<class T>
class counted_reference_t {
public:
    counted_reference_t(T *ptr) :
            ptr(ptr), counter(1) {
        ASSERT(ptr);
    }

    T *ptr;
    uint8_t counter;
};


/**Smart pointer class with reference counter. Basic implementation of
 * C++11/Boost shared_prt class.
 * usage:
    shared_ptr<cometos::Message> p1; // pointer is NULL
    // entering scope
    {
        shared_ptr<cometos::Message> p2(new Message); // p2 is owner of message
        p1=p2; // p1 is now the second owner
    }
    // p2 gets out of scope, p1 is now single owner of message
    shared_ptr<cometos::Message> p2=p1;

    // access fields via -> or *
    p1->msgOwner;
    (*p1).msgOwner;

    // message is destroyed if p1 gets out of scope, or p1 resets reference
    p1.reset();
 */
template<class T>
class shared_ptr {
public:

    shared_ptr() :
            ref(NULL) {
    }

    shared_ptr(T* ptr) {
        ref = new counted_reference_t<T>(ptr);
    }

    shared_ptr(const shared_ptr<T> &ptr) {
        ref = ptr.ref;
        if (ref) {
            ref->counter++;
        }
    }

    ~shared_ptr() {
        reset();
    }

    T& operator*() {
        ASSERT(ref);
        return *(ref->ptr);
    }
    T* operator->() {
        ASSERT(ref);
        return ref->ptr;
    }

    void operator=(const shared_ptr<T> &ptr) {
        reset();
        ref = ptr.ref;
        if (ref) {
            ref->counter++;
        }
    }

    bool operator==(const shared_ptr<T> &ptr) {
        return ref==ptr->ref;
    }

    void reset() {
        if (ref == NULL) {
            return;
        }

        ASSERT(ref->counter > 0);
        ref->counter--;
        if (ref->counter == 0) {
            delete ref->ptr;
            delete ref;
        }
        ref = NULL;
    }

private:
    counted_reference_t<T>* ref;
};

}



#endif /* POINTER_H_ */
