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
#include "device/fsl_device_registers.h"


/*MACROS---------------------------------------------------------------------*/

#define MMIO32(x) (*(volatile uint32_t*)(x))
#define PORTIDX(x) (((x)-PORTA)/(PORTB-PORTA))	 /* map PORTA,PORTB,... to 0,1,... */
#define SCGC5_MASK(x) (1<<((PORTIDX(x))+9))
#define PDOR(x) MMIO32((((PORTIDX(x))*0x10) + &GPIOA_PDOR))    /* output register address of x */
#define PSOR(x) MMIO32((((PORTIDX(x))*0x10) + &GPIOA_PSOR))    /* set register address of x */
#define PCOR(x) MMIO32((((PORTIDX(x))*0x10) + &GPIOA_PCOR))    /* clear register address of x */
#define PTOR(x) MMIO32((((PORTIDX(x))*0x10) + &GPIOA_PTOR))    /* toggle register address of x */
#define PDIR(x) MMIO32((((PORTIDX(x))*0x10) + &GPIOA_PDIR))    /* input register address of x */
#define PDDR(x) MMIO32((((PORTIDX(x))*0x10) + &GPIOA_PDDR))    /* data direction register address of x */
#define PCR(x,p) MMIO32(((PORTIDX(x))*0x400) + &PORTA_PCR0 + (p)) /* pin control register */


/**
 * This function allows to define the most basic functionality for output pins.
 * The defines allow the initialization, setting, clearing,toggling, and
 * the retrieve of the current state of the pin.
 *
 * Outpin pins can be defined in headers and afterwards included in different
 * translation units without problems. Normally the functions are placed inline.
 *
 * The use of static inline allows to store the function in function pointers, but
 * in this case the translation unit would generate a non-inlined copy of the
 * function.
 *
 * Example:
 * DEFINE_OUTPUT_PIN( led1 ,PORTA, 0 );
 * led1_init();
 * led1_toggle();
 *
 * @author Stefan Unterschuetz (for ATmega)
 * @author Florian Meier (updated for ARM)
 */

#define DEFINE_OUTPUT_PIN(NAME,PORT,P)    \
    static inline void NAME##_on()      {    PSOR(PORT) = (1 << (P));} \
    static inline void NAME##_off()     {    PCOR(PORT) = (1 << (P));} \
    static inline void NAME##_toggle()  {    PTOR(PORT) = (1 << (P));} \
    static inline void NAME##_init()    {    SIM_SCGC5 |= SCGC5_MASK(PORT); PCR(PORT,P) = 0x100; PDDR(PORT) |= (1 << (P));NAME##_off();} \
    static inline bool NAME##_get()     {    return (1&(PDOR(PORT) >> (P)));}

/** same as DEFINE_OUTPUT_PIN, but low active*/
#define DEFINE_OUTPUT_PIN_INV(NAME,PORT,P)    \
    static inline void NAME##_on()      {    PCOR(PORT) = (1 << (P));} \
    static inline void NAME##_off()     {    PSOR(PORT) = (1 << (P));} \
    static inline void NAME##_toggle()  {    PTOR(PORT) = (1 << (P));} \
    static inline void NAME##_init()    {    SIM_SCGC5 |= SCGC5_MASK(PORT); PCR(PORT,P) = 0x100; PDDR(PORT) |= (1 << (P));NAME##_off();} \
    static inline bool NAME##_get()     {    return (1&(PDOR(PORT) >> (P)));}

// defines an input pin
#define DEFINE_INPUT_PIN(NAME,PORT,P)    \
    static inline bool NAME##_read()    { return ((PDIR(PORT)&(1 << P))>0); } \
    static inline void NAME##_init()    {    SIM_SCGC5 |= SCGC5_MASK(PORT); PCR(PORT,P) = 0x100; PDDR(PORT) &= ~(1 << (P));} \
    static inline void NAME##_pullUp()  {PCR(PORT,P) |= 3; }

#endif /* PIN_H_ */
