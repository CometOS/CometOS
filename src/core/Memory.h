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
 * @author Maximilian Köstler
 */

#ifndef MEMORY_H
#define MEMORY_H

namespace cometos {

template <typename T>
class checked_ptr {
    template <typename T2, typename... A>
    friend checked_ptr<T2> make_checked(A&&... arg);

private:
    struct checked_object_wrapper {
        T* raw_instance;
        int reference_count;
    };

    checked_object_wrapper* wrapped_object;

    checked_ptr(checked_object_wrapper* const object) {
        wrapped_object = object;
    }

public:
    checked_ptr() : wrapped_object(nullptr) {
    }

    checked_ptr(const checked_ptr& other) : wrapped_object{other.wrapped_object} {
        if(this->wrapped_object) {
            this->wrapped_object->reference_count++;
        }
    }

    explicit checked_ptr(checked_ptr&& other) : wrapped_object{other.wrapped_object} {
        other.wrapped_object = nullptr;
    }

    explicit checked_ptr(T*&& raw) : checked_ptr{new checked_object_wrapper{raw, 1}} {
    }

    ~checked_ptr() {
        reset();
    }

    checked_ptr& operator=(const checked_ptr& other) {
        ASSERT(this->wrapped_object == nullptr);

        if(this->wrapped_object != other.wrapped_object) {
            this->wrapped_object = other.wrapped_object;
            if(this->wrapped_object) {
                this->wrapped_object->reference_count++;
            }
        }
        return *this;
    }

    checked_ptr& operator=(checked_ptr&& other) {
        ASSERT(this->wrapped_object == nullptr);

        this->wrapped_object = other.wrapped_object;
        other.wrapped_object = nullptr;
        return *this;
    }

    bool operator==(const checked_ptr& other) const {
        return this->wrapped_object == other.wrapped_object;
    }

    bool operator!=(const checked_ptr& other) const {
        return this->wrapped_object != other.wrapped_object;
    }

    void reset() {
        if(this->wrapped_object) {
            this->wrapped_object->reference_count--;
            ASSERT(this->wrapped_object->reference_count > 0);

            this->wrapped_object = nullptr;
        }
        return;
    }

    /**
     * This should only be used during deconstruction at the end of the runtime
     */
    void force_reset() {
        if(this->wrapped_object) {
            this->wrapped_object->reference_count--;

            if(this->wrapped_object->reference_count == 0) {
                delete this->wrapped_object->raw_instance;
                delete this->wrapped_object;
            }

            this->wrapped_object = nullptr;
        }
        return;
    }

    T* get() {
        if(this->wrapped_object) {
            return this->wrapped_object->raw_instance;
        } else {
            return nullptr;
        }
    }

    T& operator*() {
        ASSERT(this->wrapped_object != nullptr);
        ASSERT(this->wrapped_object->raw_instance != nullptr);

        return *(this->wrapped_object->raw_instance);
    }

    T* operator->() {
        ASSERT(this->wrapped_object != nullptr);
        ASSERT(this->wrapped_object->raw_instance != nullptr);

        return this->wrapped_object->raw_instance;
    }

    const T* operator->() const {
        ASSERT(this->wrapped_object != nullptr);
        ASSERT(this->wrapped_object->raw_instance != nullptr);

        return this->wrapped_object->raw_instance;
    }

    long int use_count() const noexcept {
        if(this->wrapped_object) {
            return this->wrapped_object->reference_count;
        } else {
            return 0;
        }
    }

    explicit operator bool() const noexcept {
        return this->wrapped_object != nullptr;
    }

    bool unique() const noexcept {
        return use_count() == 1;
    }

    void deleteObject() {
        ASSERT(unique());

        if(this->wrapped_object) {
            this->wrapped_object->reference_count--;

            ASSERT(this->wrapped_object->raw_instance);
            delete this->wrapped_object->raw_instance;

            delete this->wrapped_object;
            this->wrapped_object = nullptr;
        }
    }
};

template <typename T, typename... A>
checked_ptr<T> make_checked(A&&... arg) {
    return checked_ptr<T>{new T{arg...}};
}
}

#endif
