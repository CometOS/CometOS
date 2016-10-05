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

/**@file This file shows how to use the asynchronous pal firmware layer
 *
 * @author Stefan Unterschuetz
 */
#include "cometos.h"
#include "logging.h"
#include "Module.h"
#include "Airframe.h"
#include "eepromAsync.h"
#include "palLed.h"

using namespace cometos;

/*PROTOTYPES-----------------------------------------------------------------*/

uint8_t readBuffer[20];

uint8_t seg[P_FIRMWARE_SEGMENT_SIZE] = { 0 };

void test();
SimpleTask taskTest(test);

void front();
SimpleTask taskFront(front);

void front() {
	cometos::getScheduler().add(taskFront, 50);
	palLed_toggle(4);
}


void readDone(bool success) {
	cometos::getCout() << " read " << success << cometos::endl;
	for (int i = 0; i < 10; i++) {
		cometos::getCout() << " " << (int) readBuffer[i];
	}
}

void eraseDone(uint8_t code) {
	cometos::getCout() << "Erase Done " << (int) code << cometos::endl;
	eepromAsync_read(250, readBuffer, 20, readDone);
}

void writeDone(uint8_t code) {
	cometos::getCout() << "Write Done " << (int) code << cometos::endl;
	eepromAsync_read(250, readBuffer, 20, readDone);
}

void validateDone(uint8_t code) {
	cometos::getCout() << "Validate Done " << (int) code << cometos::endl;
}

void test() {
	//cometos::getCout()<<"Start Erase "<<(int)palFirmware_eraseAsync(0, eraseDone)<<cometos::endl;

	//seg[1] = 7;
	//seg[2] = 90;
	//cometos::getCout() << "Write  "<< (int) palFirmware_writeAsync(seg, 0, 1, writeDone)<< cometos::endl;

	// CRC for erased firmware is 0x0000
	cometos::getCout() << "Validate  "<< (int) palFirmware_validateAsync(0, 0, 0x0000,  validateDone)<< cometos::endl;


}

int main() {
	eepromAsync_init();
	cometos::initialize();
	cometos::getCout() << "Start" << cometos::endl;
	cometos::getScheduler().add(taskTest, 100);
	cometos::getScheduler().add(taskFront, 100);
	cometos::run();
	return 0;
}

