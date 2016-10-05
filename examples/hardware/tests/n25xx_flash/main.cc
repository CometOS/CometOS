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

/**@file Testing palRand implementation
 * @author Stefan Unterschuetz
 */
#include "cometos.h"
#include "palLed.h"
#include "Module.h"
#include "palRand.h"
#include "logging.h"
#include "n25xx.h"

using namespace cometos;


/*PROTOTYPES-----------------------------------------------------------------*/

static uint8_t writeBuffer[256];
static uint8_t readBuffer[256];

void testFlashFunction() {

	char const* text = "This is a test string for the Flash! Lets see if it is written and read correctly";

	int i = 0;
	while (text[i] != '\0') {
		writeBuffer[i] = (uint8_t) text[i];
		i++;
	}

	N25xx* flash = N25xx::getInstance();

	flash->readId(readBuffer,20);
	LOG_INFO("FLASH ID: ");

	uint8_t k;
	for ( k = 0; k < 20; k++)
		getCout() << hex <<(int) readBuffer[k] << " ";


	LOG_INFO("Writing " << (int) i << "Bytes to the buffer");

	flash->eraseSector(0xA0);
	flash->programPage(0xA0, writeBuffer);

	flash->read(0xA0, readBuffer, i);

	uint8_t j;
	for (j = 0; j < i; j++){
		getCout() << readBuffer[j];
	}
}

static SimpleTask testTask(&testFlashFunction);


int main() {
	cometos::initialize();
	palLed_on(2);

	getScheduler().add(testTask,1000);

	cometos::run();


	return 0;
}


