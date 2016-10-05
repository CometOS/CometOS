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

/**This is a full working example of a wireless node. The CometOS
 * default communication stack is included, which supports routing.
 *
 * The example also shows how to use RMI for writing user modules.
 *
 * @author Stefan Unterschuetz, Andreas Weigel
 */
/*INCLUDES-------------------------------------------------------------------*/

#include "cometos.h"
#include "palLed.h"
#include "Task.h"
#include "at86rf231_registers.h"
#include "rf231.h"
#include "cmsis_device.h"
#include "palPin.h"
#include "palSerial.h"
#include "palLocalTime.h"
#include "palTimer.h"

#define DEBUG_TIMINGS 0

#ifndef CHANNEL_TEST
#define CHANNEL_TEST 0
#endif

#ifndef SNIFF_CHANNEL
#define SNIFF_CHANNEL 24
#endif

#ifndef NUM_MEASUREMENTS
#define NUM_MEASUREMENTS 500000UL
#endif

#ifndef BAUDRATE
#define BAUDRATE 500000
#endif

#ifndef OWN_PAN_ID
#define OWN_PAN_ID 16
#endif

#define MEASUREMENT_PERIOD_US 17
#define TIME_DELAY_CHECK_ADDRESS_SHORT 75
#define TIME_DELAY_CHECK_ADDRESS_LONG 216
#define TIME_DELAY_PREAMBEL 192

#define SEQ_NO_BUFFER_SIZE 4
#define LAST_MEASUREMENTS_BUFFER_SIZE 20

#define MAX_STRING_SIZE 128

class RssiSniffer : public cometos::Module {
public:
	static bool rxInProgress;
	static bool ownNetwork;
	uint8_t channel;
	cometos::PalTimer* timer;

#if DEBUG_TIMINGS
	cometos::PalTimer* timer_debug;
#endif

	volatile uint32_t numMeasurements = 0;
	volatile uint32_t num802154RxAbove90_foreign = 0;
	volatile uint32_t num802154RxAt90_foreign = 0;
	volatile uint32_t num802154RxAbove90_own = 0;
	volatile uint32_t num802154RxAt90_own = 0;
	volatile uint32_t numAbove90 = 0;

	uint8_t seqNoIndex;
	uint8_t sequenceNumbers[SEQ_NO_BUFFER_SIZE];

	RssiSniffer() :
		cometos::Module("snif"),
		runTask(CALLBACK(&RssiSniffer::run, *this))
	{}


	bool setChannel(uint8_t channel) {
		if (11 > channel || 26 < channel){
			return false;
		}

		uint8_t reg = rf->readRegister(AT86RF231_REG_PHY_CC_CCA);
		reg = (~AT86RF231_PHY_CC_CCA_MASK_CHANNEL & reg ) | channel;
		rf->writeRegister(AT86RF231_REG_PHY_CC_CCA, reg);

		return true;
	}


	virtual void initialize() {
		serial = cometos::PalSerial::getInstance<int>(1);
		serial->init(BAUDRATE, NULL, NULL, NULL);
		uint8_t printBuffer[MAX_STRING_SIZE];

		snprintf((char*) printBuffer, MAX_STRING_SIZE, "Starting initialization\n");
		serial->write(printBuffer, strlen((char*) printBuffer));

		rf = cometos::Rf231::getInstance();

		rf->reset();
		rf->cmd_state(AT86RF231_TRX_STATE_FORCE_TRX_OFF);

		while (rf->getRfStatus() != AT86RF231_TRX_STATUS_TRX_OFF) {
			__asm("nop");
		}

#if	CHANNEL_TEST
		channel= 10;
#else
		channel = SNIFF_CHANNEL;
#endif

		// enter RX_ON state from TRX_OFF state
		rf->cmd_state(AT86RF231_TRX_STATE_RX_ON);
		while (rf->getRfStatus() != AT86RF231_TRX_STATUS_RX_ON)
			;

		// clear pending interrupts and then enable initial interrupts
		// clear pending interrupts (automatically done by reading the interrupt status register)
		rf->readRegister(AT86RF231_REG_IRQ_STATUS);

		timer = cometos::PalTimer::getInstance(5);
		timer->setFrequency(1e6);

#if DEBUG_TIMINGS
		timer_debug = cometos::PalTimer::getInstance(4);
		timer_debug->setFrequency(1e6);
#endif

		// Enable interrupts
		uint8_t irq_mask = AT86RF231_IRQ_STATUS_MASK_TRX_END | AT86RF231_IRQ_STATUS_MASK_RX_START;
		rf->writeRegister(AT86RF231_REG_IRQ_MASK, irq_mask);

		cometos::getScheduler().add(runTask);
	}


	void run() {
		numMeasurements = 0;
		num802154RxAbove90_foreign = 0;
		num802154RxAt90_foreign = 0;
		num802154RxAbove90_own = 0;
		num802154RxAt90_own = 0;
		numAbove90 = 0;
		uint8_t rssi = 0;
		bool rx = false;
		bool own = false;

#if CHANNEL_TEST
		if (channel > 25)
			channel=10;
		setChannel(++channel);
#else
		setChannel(channel);
#endif

		uint32_t startTime = palLocalTime_get();

		while(numMeasurements < NUM_MEASUREMENTS) {
			rssi = rf->readRegister(AT86RF231_REG_PHY_RSSI) & 0x1F;

			palExec_atomicBegin();
			rx = rxInProgress;
			own = ownNetwork;
			palExec_atomicEnd();

			if (rx) {
				if (own) {
					if (rssi > 0) {
						num802154RxAbove90_own++;
					} else {
						num802154RxAt90_own++;
					}
				}
				else {
					if (rssi > 0) {
						num802154RxAbove90_foreign++;
					} else {
						num802154RxAt90_foreign++;
					}
				}
			} else {
				if (rssi > 0) {
					numAbove90++;
				}
			}

			numMeasurements++;
		}

		uint32_t endTime = palLocalTime_get();
		uint32_t timeDiff = endTime - startTime;
		palLed_toggle(2);

		uint8_t buf[MAX_STRING_SIZE];
		snprintf((char*)buf, MAX_STRING_SIZE, "%8lu %8lu %8lu %8lu %8lu %8lu %8lu %d\n",endTime, numMeasurements, num802154RxAbove90_own, num802154RxAt90_own, num802154RxAbove90_foreign, num802154RxAt90_foreign, numAbove90, channel);
		serial->write(buf, strlen((char*)buf));

#if DEBUG_TIMINGS
		snprintf((char*)buf, MAX_STRING_SIZE, "%8lu\n", timeDiff);
		serial->write(buf, strlen((char*)buf));
#endif
		cometos::getScheduler().add(runTask, 0);
	}

	void initRadioInterruptPin(){
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);

		palPin_initInFloating(GPIOC,4);

		GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource4);

		EXTI_InitTypeDef intInit;
		intInit.EXTI_Line = EXTI_Line4;
		intInit.EXTI_Trigger = EXTI_Trigger_Rising;
		intInit.EXTI_Mode = EXTI_Mode_Interrupt;
		intInit.EXTI_LineCmd = ENABLE;
		EXTI_Init(&intInit);

		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

		NVIC_InitTypeDef nvic;
		nvic.NVIC_IRQChannel = EXTI4_IRQn;
		nvic.NVIC_IRQChannelCmd = ENABLE;
		nvic.NVIC_IRQChannelPreemptionPriority = 0x0F;
		nvic.NVIC_IRQChannelSubPriority = 0x0F;
		NVIC_Init(&nvic);

		EXTI_ClearITPendingBit(EXTI4_IRQn);
	}

	bool decideOwnNetwork() {
		uint8_t frameControl[2];
		uint16_t destPanId;
		uint8_t seqNo;

		//Wait at least 5 * 32 -13 us to ensure validity of adressing fields
		timer->delay(1*32);

		palExec_atomicBegin();
		rf->readSram(0x0, 2, (uint8_t*) frameControl);
		palExec_atomicEnd();


		//else frame is treated as data:
		if (frameControl[1] & 0x88){ //Destination address present
			palExec_atomicBegin();
			timer->delay(4*32 - 13);
			rf->readSram(0x3, 2, (uint8_t*) &destPanId);
			rf->readSram(0x2, 1, (uint8_t*) &seqNo);
			palExec_atomicEnd();

			//if ack is requested
			if (frameControl[0] & 0x61){
				sequenceNumbers[seqNoIndex] = seqNo;
				seqNoIndex = (seqNoIndex + 1) % SEQ_NO_BUFFER_SIZE;
			}

			if(destPanId == OWN_PAN_ID) {
				// Due to preambel sequence
				num802154RxAbove90_own += TIME_DELAY_PREAMBEL / MEASUREMENT_PERIOD_US;
				numAbove90 -= TIME_DELAY_PREAMBEL / MEASUREMENT_PERIOD_US;

				// Due to delay of this function
				numMeasurements += TIME_DELAY_CHECK_ADDRESS_LONG / MEASUREMENT_PERIOD_US;
				num802154RxAbove90_own += TIME_DELAY_CHECK_ADDRESS_LONG / MEASUREMENT_PERIOD_US;

#if DEBUG_TIMINGS
				uint16_t microseconds = timer_debug->get();
				uint8_t buf[MAX_STRING_SIZE];
				snprintf((char*)buf, MAX_STRING_SIZE, "longown: %d\n", microseconds);
				serial->write(buf, strlen((char*)buf));
				timer_debug->stop();
#endif
				return true;
			}
			else {
				// Due to preambel sequence
				num802154RxAbove90_foreign += TIME_DELAY_PREAMBEL / MEASUREMENT_PERIOD_US;
				/*It can happen, that not all measurements during the preamble were over 90,
			      and then numAbove90 ends up wrapping around and being a huge number. Therefore
				  we just set it to 0 in that case. Better would be, to keep a Buffer of the last
				  measured values and check how many to substract here!
				*/
				if(numAbove90 < TIME_DELAY_PREAMBEL / MEASUREMENT_PERIOD_US) {
					numAbove90 = 0;
				}
				else {
					numAbove90 -= TIME_DELAY_PREAMBEL / MEASUREMENT_PERIOD_US;
				}

				// Due to delay of this function
				numMeasurements += TIME_DELAY_CHECK_ADDRESS_LONG / MEASUREMENT_PERIOD_US;
				num802154RxAbove90_foreign += TIME_DELAY_CHECK_ADDRESS_LONG / MEASUREMENT_PERIOD_US;

#if DEBUG_TIMINGS
				uint16_t microseconds = timer_debug->get();
				uint8_t buf[MAX_STRING_SIZE];
				snprintf((char*)buf, MAX_STRING_SIZE, "longforeign: %d\n", microseconds);
				serial->write(buf, strlen((char*)buf));
				timer_debug->stop();
#endif

				return false;
			}
		}

		//if frame is Acknowledgement, its own traffic:
		if (frameControl[0] & 0x02) {
			uint8_t ackedSeqNo = 0;
			rf->readSram(0x2, 1, (uint8_t*) &ackedSeqNo);
			for (uint8_t i = 0; i < SEQ_NO_BUFFER_SIZE; i++) {
				if (ackedSeqNo == sequenceNumbers[i]) {
					// Due to preambel sequence
					num802154RxAbove90_own += TIME_DELAY_PREAMBEL / MEASUREMENT_PERIOD_US;
					numAbove90 -= TIME_DELAY_PREAMBEL / MEASUREMENT_PERIOD_US;

					// Due to delay of this function
					numMeasurements += TIME_DELAY_CHECK_ADDRESS_SHORT / MEASUREMENT_PERIOD_US;
					num802154RxAbove90_own += TIME_DELAY_CHECK_ADDRESS_SHORT / MEASUREMENT_PERIOD_US;
					return true;
				}
			}
		}


		// else its foreign traffic
		// Due to preambel sequence
		num802154RxAbove90_foreign += TIME_DELAY_PREAMBEL / MEASUREMENT_PERIOD_US;
		numAbove90 -= TIME_DELAY_PREAMBEL / MEASUREMENT_PERIOD_US;
		// Due to delay of this function
		numMeasurements += TIME_DELAY_CHECK_ADDRESS_SHORT / MEASUREMENT_PERIOD_US;
		num802154RxAbove90_foreign += TIME_DELAY_CHECK_ADDRESS_SHORT / MEASUREMENT_PERIOD_US;

#if DEBUG_TIMINGS
		uint16_t microseconds = timer_debug->get();
		uint8_t buf[MAX_STRING_SIZE];
		snprintf((char*)buf, MAX_STRING_SIZE, "short: %d\n", microseconds);
		serial->write(buf, strlen((char*)buf));
		timer_debug->stop();
#endif

		return false;
	}

	void printSram() {
		uint8_t buffer[16];
		uint8_t printBuffer[MAX_STRING_SIZE];

		palExec_atomicBegin();
		cometos::Rf231::getInstance()->readSram(0x0, 16, buffer);
		palExec_atomicEnd();

		snprintf((char*) printBuffer, MAX_STRING_SIZE, "SRAM: %02x%02x%02x%02x%02x%02x%02x%02x\n",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6],buffer[7] );

		serial->write(printBuffer, strlen((char*) printBuffer));
	}

private:
	cometos::CallbackTask runTask;
	cometos::PalSerial* serial;
	cometos::Rf231* rf;
};

bool RssiSniffer::rxInProgress = false;
bool RssiSniffer::ownNetwork = false;
static RssiSniffer sniffy;


int main() {

	cometos::initialize();
	cometos::run();

	return 0;
}

#ifdef __cplusplus
extern "C" {
#endif

//External interrupt used by the RF231
void EXTI4_IRQHandler() {

	NVIC_DisableIRQ(EXTI4_IRQn);
	EXTI_ClearITPendingBit(EXTI_Line4);

#if DEBUG_TIMINGS
	cometos::PalTimer::getInstance(4)->start_async(0xFFFF, NULL);
#endif

	uint8_t irq =  cometos::Rf231::getInstance()->readRegister(AT86RF231_REG_IRQ_STATUS);

	if (irq & AT86RF231_IRQ_STATUS_MASK_RX_START) {

		RssiSniffer::rxInProgress = true;
		RssiSniffer::ownNetwork = sniffy.decideOwnNetwork();
	}

	if (irq & AT86RF231_IRQ_STATUS_MASK_TRX_END) {
		RssiSniffer::rxInProgress = false;
		RssiSniffer::ownNetwork = false;
		//sniffy.printSram();
	}
	NVIC_EnableIRQ(EXTI4_IRQn);
}

#ifdef __cplusplus
}
#endif

