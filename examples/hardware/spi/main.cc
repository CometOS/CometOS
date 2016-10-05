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

/**@file Example for using only CometOS scheduler (without core framework).
 *
 * @author Stefan Unterschuetz
 */

/*INCLUDES-------------------------------------------------------------------*/

#define BAUD 9600

#include "cmsis_device.h"
#include "pal.h"
#include "palSerial.h"
#include "palExec.h"
#include "palLed.h"
#include "TaskScheduler.h"
#include "logging.h"
#include "rf231.h"
#include "OutputStream.h"

using namespace cometos;

/*PROTOTYPES-----------------------------------------------------------------*/

void sendBack();

uint8_t receive[128];
uint8_t send[128];
uint16_t fillSize;
PalSerial* uart;
PalSpi* spi;

TaskScheduler scheduler;

void sendBack() {
	/*if (fillSize > 0)
	{
		fillSize -= uart->write(receive, fillSize, false);
	}
	*/

}

int main() {

	pal_init();

	send[0] = 0x1c | (1 << 7);

	uart = PalSerial::getInstance(1);

	while (1){


		receive[0] = Rf231::getInstance()->readRegister(RF2XX_REG_PART_NUM);
		receive[1] = Rf231::getInstance()->readRegister(RF2XX_REG_VERSION_NUM);
		receive[2] = Rf231::getInstance()->readRegister(RF2XX_REG_MAN_ID_0);
		receive[3] = Rf231::getInstance()->readRegister(RF2XX_REG_MAN_ID_1);

		cometos::getCout() << "PartNum: " << receive[0] << "\n"
							<< "Version Num: " << receive[1] << "\n"
							<< "ManId: " << *((uint16_t*) (receive + 2)) << "\n";

		//fillSize = 4;
		//sendBack();
		palLed_toggle(4);

		palExec_sleep(2000);
	}

	return 0;
}




