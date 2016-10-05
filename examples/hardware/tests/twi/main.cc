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

/**@file Testing TWI/EEPRM in Interrupt Mode
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

void read();
SimpleTask taskRead(read);
void write();
SimpleTask taskWrite(write);


uint8_t readBuffer[10];
uint8_t writeBuffer[10]={0};

void readDone(bool success)  {
	cometos::getCout()<<" send success "<<success<<cometos::endl;
	 for (int i=0;i<10;i++) {
			 cometos::getCout()<<" "<<(int)readBuffer[i];
		 }
}

void writeDone(bool success)  {
	cometos::getCout()<<" write success "<<success<<cometos::endl;
}

void read() {
	 cometos::getCout()<<cometos::endl<<"Read data "<<cometos::endl;
	 eepromAsync_read(250,readBuffer,10,readDone);
	 cometos::getScheduler().add(taskWrite,2000);
}

void write() {
	 cometos::getCout()<<cometos::endl<<"Write data "<<cometos::endl;
	 for (int i=0;i<10;i++) {
		 writeBuffer[i]=writeBuffer[i]+i;
	 }
	 eepromAsync_write(250,writeBuffer,10,writeDone);
	 //eepromAsync_write(256,writeBuffer+6,4,writeDone);

	 cometos::getScheduler().add(taskRead,5000);
}


int main() {
	eepromAsync_init();
    cometos::initialize();
    cometos::getCout()<<"Start"<<cometos::endl;
    cometos::getScheduler().add(taskRead,100);
    cometos::run();
    return 0;
}


#ifdef __cplusplus
extern "C" {
#endif



#ifdef __cplusplus
}
#endif

