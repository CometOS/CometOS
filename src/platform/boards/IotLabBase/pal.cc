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

#include "palExec.h"
#include "pal.h"
#include "palLed.h"
#include "palSerial.h"
#include "palGPIOPin.h"
#include "palId.h"
#include "palPin.h"
#include "palTimerImp.h"
#include "palRand.h"
#include "palSpi.h"
#include "rf231.h"
#include "n25q128a.h"
#include "ExceptionHandlers.h"

#include "cmsis_device.h"

#ifndef SERIAL_BAUDRATE
#define SERIAL_BAUDRATE 500000
#endif

#ifndef RF_SPI_PORT
#define RF_SPI_PORT 1
#endif

#ifndef EXTERNAL_FLASH
#define EXTERNAL_FLASH 0
#endif

using namespace cometos;


static IGPIOPinImpl radio_spi_cs(GPIOA, 4);
static IGPIOPinImpl radio_rst(GPIOC, 1);
static IGPIOPinImpl radio_slptr(GPIOA, 2);
static IGPIOPinImpl radio_irq(GPIOC, 4);

static IGPIOPinImpl flash_spi_cs(GPIOA, 11);
static IGPIOPinImpl flash_writeLock(GPIOC,6);
static IGPIOPinImpl flash_hold(GPIOC, 9);

void initRadioInterruptPin();

void init_uart(){

    // init UART
    cometos::PalSerial::getInstance(1)->init(SERIAL_BAUDRATE, 0, 0, 0);

    //Enable clock for PORTA
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* Configure USART Tx as alternate function push-pull */
    palPin_initAFPushPull(GPIOA,9);

    /* Configure USART Rx as input floating */
    palPin_initInFloating(GPIOA,10);

    //Enable clock for alternate function
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);;
}


void init_radio(){
    //init gpio and alternate function clock
#if RF_SPI_PORT == 1
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    //Configure SCLK & MOSI as alternate function push pull
    palPin_initAFPushPull(GPIOA, 5);
    palPin_initAFPushPull(GPIOA, 7);

    //Configure MISO as input floating
    palPin_initInFloating(GPIOA, 6);
#else
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    //Configure SCLK & MOSI as alternate function push pull
    palPin_initAFPushPull(GPIOB, 13);
    palPin_initAFPushPull(GPIOB, 15);

    //Configure MISO as input floating
    palPin_initInFloating(GPIOB, 14);
#endif

    PalSpi* spi =  PalSpi::getInstance(RF_SPI_PORT);

    spi->setCsPin(&radio_spi_cs, true);
    spi->init(true, 4e6, 0, false);

    Rf231* radio = Rf231::getInstance();
    radio->init(spi, &radio_rst, &radio_slptr, &radio_irq, PalTimer::getInstance(Timer::RADIO));

    initRadioInterruptPin();
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

#if EXTERNAL_FLASH
void initExternalFlash() {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC
            | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB
            | RCC_APB2Periph_AFIO, ENABLE);

    //Configure SCLK & MOSI as alternate function push pull
    palPin_initAFPushPull(GPIOB, 13);
    palPin_initAFPushPull(GPIOB, 15);

    //Configure MISO as input floating
    palPin_initInFloating(GPIOB, 14);

    PalSpi* spi2 =  PalSpi::getInstance(2);
    spi2->init(true, 18e6, 0, false);
    spi2->setCsPin(&flash_spi_cs, true);
    spi2->disable();

    N25xx* flash = N25xx::getInstance();
    flash->init(spi2, &flash_hold, &flash_writeLock);
}
#endif

#ifdef SERIAL_PRINTF
#include "OutputStream.h"

static bool blockingCout = true;

void setBlockingCout(bool value) {
    blockingCout = value;
}

static void serial_putchar(char c) {
    if(blockingCout) {
	    // Wait for empty transmit buffer
	    while (!(USART1->SR & USART_SR_TXE))
		    ;
	    palExec_atomicBegin();
	    while (!(USART1->SR & USART_SR_TXE))
		    ;
	    USART1->DR = c;
	    palExec_atomicEnd();
    }
    else {
        cometos::PalSerial::getInstance(1)->write( (uint8_t*) &c, 1);
    }
}

namespace cometos {

static OutputStream cout(serial_putchar);

OutputStream  &getCout() {
    static bool init=false;
    if (init==false) {
        init=true;
        SystemCoreClockUpdate();
        init_uart();
    }

    return cout;
}

}
#else

void setBlockingCout(bool value) {
}

#endif

void pal_init() {
    SystemCoreClockUpdate();

    palExec_init();
    palLed_init();
    palId_init();
    init_uart();

    init_radio();

#if EXTERNAL_FLASH
    initExternalFlash();
#endif

    palRand_init();
    srand(palRand_get());

}

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MANUAL_MAC_INIT

//External interrupt used by the RF231
void EXTI4_IRQHandler() {
	NVIC_DisableIRQ(EXTI4_IRQn);
	EXTI_ClearITPendingBit(EXTI_Line4);
    Rf231::getInstance()->triggerInterrupt();
    NVIC_EnableIRQ(EXTI4_IRQn);
}


#endif

#ifdef __cplusplus
}
#endif
