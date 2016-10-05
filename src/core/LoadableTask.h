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

#ifndef LOADABLE_TASK
#define LOADABLE_TASK

#include "Task.h"
#include "Tuple.h"

namespace cometos {

template<int... Is>
struct seq { };

template<int N, int... Is>
struct gen_seq : gen_seq<N - 1, N - 1, Is...> { };

template<int... Is>
struct gen_seq<0, Is...> : seq<Is...> { };

template<typename... Params>
class LoadableTask: public Task {
public:
    LoadableTask(Callback<void(Params...)> callback = EMPTY_CALLBACK())
    : callback(callback) {
    }

    Callback<void(Params...)> getCallback() {
        return callback;
    }

    void setCallback(Callback<void(Params...)> callback) {
        this->callback = callback;
    }

    void load(Params... params)
    {
        Tuple<Params...> t(params...);
        parameters = t;
    }

    template<int... Is>
    void run(seq<Is...>) {
        callback(TupleGetter<Is,Params...>::get(parameters)...);
    }

    virtual void invoke() {
        if(callback) {
            run(gen_seq<sizeof...(Params)>());
        }
    }

private:
   Callback<void(Params...)> callback;
   Tuple<Params...> parameters;
};

}

#endif
