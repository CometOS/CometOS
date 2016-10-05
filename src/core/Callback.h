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

/*
 * Based on C++ Delegates On Steroids
 * Imported at 08.04.2015 and adapted to CometOS
 *
 * by Oliver Mueller <oliver.mueller@gmail.com>
 * https://github.com/marcmo/delegates
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 oliver.mueller@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef CALLBACK_H_
#define CALLBACK_H_

#include "cometosAssert.h"

/**
 * Main template for delgates
 *
 * \tparam return_type  return type of the function that gets captured
 * \tparam params       variadic template list for possible arguments
 *                      of the captured function
 */

namespace cometos {

class Task;

template<typename... Params>
class LoadableTask;

class EmptyCallback
{
};

template<typename return_type, typename... params>
class Callback; //forward declaration..

template<typename return_type, typename... params>
class Callback<return_type(params...)>
{
    typedef return_type (*Pointer2Function)(void* callee, params...);
public:
    Callback(void* callee, Pointer2Function function)
        : fpCallee(callee)
        , fpCallbackFunction(function)
    {}

    Callback()
	: fpCallee(nullptr)
        , fpCallbackFunction(nullptr)
    {
    }

    Callback(const EmptyCallback& empty)
	: fpCallee(nullptr)
        , fpCallbackFunction(nullptr)
    {
    }

    Callback(Task* task);

    operator bool() const
    {
	return (fpCallbackFunction != nullptr);
    }

    return_type operator()(params... xs) const
    {
	ASSERT(fpCallbackFunction != nullptr);

        return (*fpCallbackFunction)(fpCallee, xs...);
    }

    bool operator==(const Callback& other) const
    {
        return (fpCallee == other.fpCallee)
               && (fpCallbackFunction == other.fpCallbackFunction);
    }

    Callback& operator=(const EmptyCallback& empty)
    {
	fpCallee = nullptr;
	fpCallbackFunction = nullptr;
	return *this;
    }

    Callback& operator=(Task* task);

    void* getCallee() {
        return fpCallee;
    }

    Pointer2Function getFunction() {
        return fpCallbackFunction;
    }

private:

    /**
     * Using void here is probably safe (contrary to the common UntypedDelegate),
     * since it is always casted to T* (of T::xx()) in CallbackFactory::Create(T* o) first,
     * before casting it to void.
     */
    void* fpCallee;
    Pointer2Function fpCallbackFunction;
};


/**
 * A CallbackFactory is used to create a Callback for a certain method call.
 * It takes care of setting up the function that will cast the object that is stored
 * inside the Callback back to the correct type.
 */
template<typename T, typename return_type, typename... params>
struct CallbackFactory
{
    template<return_type (T::*Func)(params...)>
    static return_type MethodCaller(void* o, params... xs)
    {
        return (static_cast<T*>(o)->*Func)(xs...);
    }

    template <return_type (*TFnctPtr)(params...)>
	static return_type FunctionCaller(void*, params... xs)
	{
		return (TFnctPtr)(xs...);
	}

    template<return_type (T::*Func)(params...)>
    inline static Callback<return_type(params...)> Create(T* o)
    {
        return Callback<return_type(params...)>(o, &CallbackFactory::MethodCaller<Func>);
    }

    template<return_type (*TFnctPtr)(params...)>
    inline static Callback<return_type(params...)> CreateForFunction()
    {
        return Callback<return_type(params...)>(0L, &CallbackFactory::FunctionCaller<TFnctPtr>);
    }
};
/**
 * helper function that is used to deduce the template arguments.
 * will return a CallbackFactory
 */
template<typename T, typename return_type, typename... params>
CallbackFactory<T, return_type, params... > MakeCallback(return_type (T::*)(params...))
{
    return CallbackFactory<T, return_type, params...>();
}
class no_type{};
template<typename return_type, typename... params>
CallbackFactory<no_type, return_type, params... > MakeCallback2(return_type (*TFnctPtr)(params...))
{
    return CallbackFactory<no_type, return_type, params...>();
}

#define CALLBACK_MET(func, thisPrtRef) (MakeCallback(func).Create<func>(&thisPrtRef))
#define CALLBACK_FUN(func) (MakeCallback2(func).CreateForFunction<func>())
#define EMPTY_CALLBACK() (EmptyCallback())


/***
 * TOLERANT CALLBACK
 * A callback with no parameters than can be called with parameters that will be ignored.
 * This is especially interesting when you need something to be done at the end (e.g. releasing a resource)
 * but you are not interested in the results.
 *
 * TODO There might be a more elegant version, but for now it works.
 */
template<typename T, typename return_type, typename... params>
struct TolerantCallbackFactory
{
    template<return_type (T::*Func)()>
    static return_type TolerantMethodCaller(void* o, params... xs)
    {
        return (static_cast<T*>(o)->*Func)();
    }

    template<return_type (T::*Func)()>
    inline static Callback<return_type(params...)> Create(T* o)
    {
        return Callback<return_type(params...)>(o, &TolerantCallbackFactory::TolerantMethodCaller<Func>);
    }
};

template<typename... params>
struct TolerantCallbackFactoryFactory {
    template<typename T, typename return_type>
    inline static TolerantCallbackFactory<T, return_type, params...> MakeTolerantCallback(return_type (T::*)())
    {
        return TolerantCallbackFactory<T, return_type, params...>();
    }
};

#define TOLERANT_CALLBACK(func, thisPrtRef, params) (TolerantCallbackFactoryFactory<params>().MakeTolerantCallback(func).template Create<func>(&thisPrtRef))

}

#endif

