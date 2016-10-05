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

#include "palGPIOPin.h"
#include "cometos.h"

using namespace std;

namespace cometos {

cometos_error_t GPIOPin::exportPin() {
	// open gpio export
	ofstream f("/sys/class/gpio/export");
	if(!f.is_open()) {
		std::cerr << "Could not open GPIO export." << std::endl;
		return COMETOS_ERROR_FAIL;
	}

	// export pin
	f << pin << std::endl;
	if(!f.good()) {
                return COMETOS_ERROR_BUSY;
        }

        return COMETOS_SUCCESS;
}

cometos_error_t GPIOPin::unexportPin() {
	// open gpio export
	ofstream f("/sys/class/gpio/unexport");
	if(!f.is_open()) {
		std::cerr << "Could not open GPIO unexport." << std::endl;
		return COMETOS_ERROR_FAIL;
	}

	// unexport pin
	f << pin << std::endl;
	if(!f.good()) {
                return COMETOS_ERROR_BUSY;
        }

        return COMETOS_SUCCESS;
}

GPIOPin::GPIOPin(int pin)
: pin(pin), valid(false), monitorRunning(true) {
        cometos_error_t ret = exportPin();

        if(ret == COMETOS_ERROR_BUSY) {
		std::cerr << "GPIO pin does not exist or is in use." << std::endl;
		std::cerr << "I am trying to unexport it first." << std::endl;

                ret = unexportPin();

                if(ret == COMETOS_SUCCESS) {
                        ret = exportPin();
                }
	}

        if(ret != COMETOS_SUCCESS) {
		std::cerr << "Could not initialize GPIO pin." << std::endl;
                return;
        }

	// initialize pipe for interrupting poll
	if(pipe(pipefds) < 0) {
		std::cerr << "Could not initialize poll." << std::endl;
		return;
   	}

	// everything was initialized correctly
	valid = true;

	// start monitor thread
	monitorThread = thread(&GPIOPin::monitorPin, this);
}

GPIOPin::~GPIOPin() {
            std::cout << "Delete Pin!" << std::endl;
	if(valid) {
		// close monitor thread
		monitorRunning = false;
		write(pipefds[1], ".", 1); // write to pipe to interrupt poll
		monitorThread.join();

		// unexport pin
                unexportPin();
	}
}

bool GPIOPin::is_valid() {
	return valid;
}

void GPIOPin::setDirection(Direction direction) {
	if(!valid) {
		throw invalidException;
	}

	stringstream fn;
	fn << "/sys/class/gpio/gpio" << pin << "/direction";

	ofstream f(fn.str());
	if(direction == Direction::OUT) {
		f << "out" << std::endl;
	}
	else {
		f << "in" << std::endl;
	}
}

uint8_t GPIOPin::get() {
	if(!valid) {
		throw invalidException;
	}

	stringstream fn;
	fn << "/sys/class/gpio/gpio" << pin << "/value";

	ifstream f(fn.str());

	char value;
	f >> value;

	return value=='1' ? 1 : 0;
}

cometos_error_t GPIOPin::setupInterrupt(Edge type, Callback<void(void)> callback) {
	if(!valid) {
		throw invalidException;
	}

	stringstream fn;
	fn << "/sys/class/gpio/gpio" << pin << "/edge";
	ofstream f(fn.str());

	if(!f.is_open()) {
		std::cerr << "Pin does not support interrupts" << std::endl;
		return COMETOS_ERROR_FAIL;
	}

	switch(type) {
		case Edge::NONE:
			f << "none";
			break;
		case Edge::RISING:
			f << "rising";
			break;
		case Edge::FALLING:
			f << "falling";
			break;
		case Edge::ANY_EDGE:
			f << "both";
			break;
        default:
            return COMETOS_ERROR_FAIL;
	}

        this->callback = callback;

        return COMETOS_SUCCESS;
}

void GPIOPin::monitorPin() {
	stringstream fn;
	fn << "/sys/class/gpio/gpio" << pin << "/value";

	// open file descriptor
	int gpio_fd = open(fn.str().c_str(), O_RDONLY);

	// read to clear interrupt
	char c;
	read(gpio_fd, &c, 1);
	
	// setup poll descriptor
	struct pollfd fdset[2];
	fdset[0].fd = pipefds[0];
	fdset[0].events = POLLIN;
	fdset[1].fd = gpio_fd;
	fdset[1].events = POLLPRI;

	while(monitorRunning) {
		// block until either a GPIO interrupt occurs
		// or something is written to the pipe
		if(poll(fdset, 2, -1) < 0) {
			std::cerr << "poll() failed" << std::endl;
			return;
		}
            
		// check if there was a GPIO interrupt
		if (fdset[1].revents & POLLPRI) {
			// read to clear interrupt
			read(fdset[1].fd, &c, 1);

			if(callback) {
	            		//cometos::getScheduler().replace(*callback); // TODO was this better?
				callback();
			}
		}
	}
}


void GPIOPin::setValue(int val) {
	if(!valid) {
		throw invalidException;
	}

	stringstream fn;
	fn << "/sys/class/gpio/gpio" << pin << "/direction";

	ofstream f(fn.str());
        f << val << std::endl;
}

void GPIOPin::set() {
        setValue(1);
}

void GPIOPin::clear() {
        setValue(0);
}

}
