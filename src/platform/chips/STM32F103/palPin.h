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

#ifndef PIN_H_
#define PIN_H_

/*INCLUDES-------------------------------------------------------------------*/


#include <stdint.h>
#include <stdbool.h>
#include "cmsis_device.h"


/*
 * The STM32 has 4 configuration bits per PIN -> 4 * 16 = 64 bit register
 *
 * Instead of using these MACROS, one can also use the functions defined below
 */

#define GPIOx(_N)                 ((GPIO_TypeDef *)(GPIOA_BASE + (GPIOB_BASE-GPIOA_BASE)*(_N)))
#define PIN_MASK(_N)              (1 << (_N))
#define RCC_MASKx(_N)             (RCC_APB2Periph_GPIOA << (_N))



#define DEFINE_OUTPUT_PIN(NAME,X,P) \
		static inline void NAME##_on()      {    GPIOx(X)->BSRR = (1 << (P));} \
		static inline void NAME##_off()     {    GPIOx(X)->BSRR = (1 << ((P) + 16));} \
		static inline void NAME##_toggle()  {    GPIOx(X)->ODR ^= (1 << (P));} \
		static inline void NAME##_init()    {    \
			RCC_APB2PeriphClockCmd(RCC_MASKx(X), ENABLE); \
			GPIO_InitTypeDef GPIO_InitStructure; \
			GPIO_InitStructure.GPIO_Pin = PIN_MASK(P); \
			GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; \
			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; \
			GPIO_Init(GPIOx(X), &GPIO_InitStructure); \
			NAME##_off(); }\
		static inline bool NAME##_get()     {    return (GPIOx(X)->ODR & (1 << (P)));}


#define DEFINE_OUTPUT_PIN_INV(NAME,X,P)    \
		static inline void NAME##_on()      {    GPIOx(X)->BSRR = (1 << ((P) + 16));} \
		static inline void NAME##_off()     {    GPIOx(X)->BSRR = (1 << (P));} \
		static inline void NAME##_toggle()  {    GPIOx(X)->ODR ^= (1 << (P));} \
		static inline void NAME##_init()    {    \
			RCC_APB2PeriphClockCmd(RCC_MASKx(X), ENABLE); \
			GPIO_InitTypeDef GPIO_InitStructure; \
			GPIO_InitStructure.GPIO_Pin = PIN_MASK(P); \
			GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; \
			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; \
			GPIO_Init(GPIOx(X), &GPIO_InitStructure);\
			NAME##_off(); }\
		static inline bool NAME##_get()     {    return (GPIOx(X)->ODR & (1 << (P)));}

// defines an input pin
#define DEFINE_INPUT_PIN(NAME,X,P)    \
		static inline bool NAME##_read()    { return GPIOx(X)->IDR & (1 << (P)); } \
		static inline void NAME##_init()    {    \
			RCC_APB2PeriphClockCmd(RCC_MASKx(X), ENABLE); \
			GPIO_InitTypeDef GPIO_InitStructure; \
			GPIO_InitStructure.GPIO_Pin = PIN_MASK(P); \
			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; \
			GPIO_Init(GPIOx(X), &GPIO_InitStructure);}\
		static inline void NAME##_pullUp()  { uint64_t *reg = (uint64_t*) &(GPIOx(X)->CRL); *reg |= (uint64_t) (0xC << (4 * (P))); GPIOx(X)->ODR >= (1 << (P)); NAME##_off(); }


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes a Pin as alternative function pin and activates pull-up / pull-down
 *
 * @param port Pin Port (e.g. GPIOB)
 * @param pin Pin number (between 1 and 16)
 */
void palPin_initAFPushPull(GPIO_TypeDef* port, uint16_t pin);

/**
 * Initializes a pin as input floating
 *
 * @param port Pin Port (e.g. GPIOB)
 * @param pin Pin number (between 1 and 16)
 *
 */
void palPin_initInFloating(GPIO_TypeDef* port, uint16_t pin);

/**
 * Initializes a pin as output floating
 *
 * @param port Pin Port (e.g. GPIOB)
 * @param pin Pin number (between 1 and 16)

 */
void palPin_initOutputPushPull(GPIO_TypeDef* port, uint16_t pin);

#ifdef __cplusplus
}
#endif

#endif /* PIN_H_ */
