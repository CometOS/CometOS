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

/**
 * @author Stefan Unterschuetz (for AVR)
 * @author Milad Bazzi (for ARM)
 * @author Florian Meier (for ARM)
 */

#include "pal.h"
#include "palExec.h"
#include <string.h>

#include "palRand.h"
#include <stdlib.h>

#ifdef PAL_ID
#include "palId.h"
#endif

#ifdef PAL_LED
#include "palLed.h"
#endif

#ifdef PAL_PERS
#include "palPers.h"
#endif

#if defined PAL_FIRMWARE || defined PAL_FIRMWARE_ASYNC
#include "palFirmware.h"
#endif

#include "palPin.h"

#ifdef __cplusplus
extern "C" {
#endif
void __cxa_pure_virtual() {
}
#ifdef __cplusplus
}
#endif


#ifdef FNET
extern "C"
{
#include "fnet.h"
#include "fnet_cpu.h"
}
#endif


#include "OutputStream.h"
#ifdef SERIAL_PRINTF

static void serial_init() {
	SIM_SCGC4 |= SIM_SCGC4_UART0_MASK; //UART0
	SIM_SCGC5 |= SIM_SCGC5_PORTB_MASK; //Port B for UART0

	PORTB_PCR17 |= PORT_PCR_MUX(3);
	PORTB_PCR16 |= PORT_PCR_MUX(3);

	UART0_C2 &= ~(UART_C2_TE_MASK | UART_C2_RE_MASK );
	UART0_C1 = 0; /*Default settings of the register*/

	//UART baud rate = UART module clock/(Baud rate * 16)
	// UART0 and UART1 are clocked from the CPU clock - all others with fnet_mk_periph_clk_khz();
	uint16_t ubd1 = (uint16_t)((F_CPU)/(SERIAL_PRINTF*16));	/* Calculate baud settings */

	//Set UART_BDH - UART_BDL register
	uint8_t temp1 = UART0_BDH & ~(UART_BDH_SBR(0x1F));/*Save the value of UART0_BDH except SBR*/
	//concatenate ubd and temp to UARTx_BDH (3 bits from temp + 5 bits from ubd)
	UART0_BDH = temp1 | (((ubd1 & 0x1F00) >> 8));
	UART0_BDL = (uint8_t)(ubd1 & UART_BDL_SBR_MASK);

	UART0_C2 |=(UART_C2_TE_MASK |UART_C2_RE_MASK); /* Enable receiver and transmitter*/
}

static void serial_putchar(char c) {
#if 0
	// Wait for empty transmit buffer
	while (!(UCSR0A & (1 << UDRE0)))
		;
	UDR0 = c;
#endif

	/* Wait until space is available in the FIFO */
	while(!(UART0_S1 & UART_S1_TDRE_MASK));
	/* Send the character */
	UART0_D = (uint8_t)c;

}

namespace cometos {
OutputStream  &getCout() {
    static OutputStream cout(serial_putchar);
    static bool init=false;
    if (init==false) {
        init=true;
        serial_init();
    }
return cout;
}

}


#endif



void pal_init() {
#ifdef PAL_LED
    palLed_init();
#endif
#ifdef PAL_ID
    palId_init();
#endif
#ifdef PAL_RAND
    palRand_init();
    srand(palRand_get());
#elif defined PAL_ID
    srand(palId_id());
#else
    srand(0);
#endif
#ifdef PAL_PERS
	palPers_init();
#endif
#if defined PAL_FIRMWARE || defined PAL_FIRMWARE_ASYNC
	if (PAL_FIRMWARE_SUCCESS != palFirmware_init()) {
	    palLed_on(4);
	}
#endif
	palExec_init();

#ifdef FNET
	/* Ethernet via FNET */
	fnet_cpu_irq_enable(0);
	if (fnet_init_static() == FNET_ERR) 
	{
		cometos::getCout() << "Error:TCP/IP stack initialization has failed." << cometos::endl;
	}
#endif
}

extern "C" {
node_t palId_id() {
#ifdef FNET
	fnet_netif_desc_t netif = fnet_netif_get_default();

	// throw away the upper bits
	return (uint16_t)fnet_netif_get_ip4_addr(netif);
#else
	return 0;
#endif
}

const char* palId_name() {
	return "CometOS Build " __TIMESTAMP__;
}
}

uint8_t heapGetUtilization() {
	return 0;
}

