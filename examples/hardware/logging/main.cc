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

/**@file This example shows how to use the CometOS logging facility.
 * 		 Note that logging can not be used in interrupts.
 *
 * @author Stefan Unterschuetz
 */
#include "cometos.h"
#include "logging.h"
#include "Task.h"
#include "Module.h"

/*PROTOTYPES-----------------------------------------------------------------*/

class SomeClass: cometos::Module {
public:
	SomeClass(const char* name) :
		cometos::Module(name) {
	}
	void initialize() {
		cometos::Module::initialize();
		LOG_DEBUG("Init "<<getName()<<cometos::endl);
		this->setLogLevel(LOG_LEVEL_ERROR);
		schedule(new cometos::Message, &SomeClass::timeout,500);
	}

	void timeout(cometos::Message *msg) {
		static uint8_t val=0;
		LOG_INFO("I:"<<getName()<<" "<<val<<cometos::endl);
		LOG_ERROR("E:"<<getName()<<" "<<val<<cometos::endl);
		val++;
		schedule(msg, &SomeClass::timeout,1500);
	}
};

void someFunction();

cometos::SimpleTask someTask(someFunction);
SomeClass someClass("sc");

void someFunction() {
	static uint8_t counter = 0;
	LOG_DEBUG("D:someFunc: counter= "<<counter<<cometos::endl);
	LOG_WARN("W:someFunc: counter= "<< counter<<cometos::endl);
	counter++;
	cometos::getScheduler().add(someTask, 1000);
}

int main() {

	someTask.setLogLevel(LOG_LEVEL_WARN);

	cometos::initialize();

	LOG_DEBUG("Start Application\n");

	cometos::getScheduler().add(someTask);
	cometos::run();
	return 0;
}
