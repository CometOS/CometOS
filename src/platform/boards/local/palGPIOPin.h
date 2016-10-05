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

#include <iostream>
#include <sstream>
#include <fcntl.h>    
#include <unistd.h>
#include <thread>
#include <fstream>
#include <poll.h>
#include <exception>
#include <functional>
#include <csignal>

#include "Callback.h"
#include "IGPIOPin.h"

namespace cometos {

class GPIOPin : public IGPIOPin {
public:
	GPIOPin(int pin);
	~GPIOPin();
	bool is_valid();

	void setDirection(Direction direction);
	virtual uint8_t get();

	virtual cometos_error_t setupInterrupt(Edge type, Callback<void(void)> callback);

        virtual void set();
        virtual void clear();

        GPIOPin(const GPIOPin& that) = delete;
        GPIOPin(const GPIOPin&& that) = delete;

private:
	int pin;
	bool valid;
        Callback<void(void)> callback;

        std::thread monitorThread;
	int pipefds[2];
	bool monitorRunning;

	void monitorPin();

	class GPIOInvalidException : public std::exception {
		virtual const char* what() const throw() {
			return "The GPIO pin was not initialized correctly.";
		}
	} invalidException;

        void setValue(int val);

        cometos_error_t exportPin();
        cometos_error_t unexportPin();
};

}

