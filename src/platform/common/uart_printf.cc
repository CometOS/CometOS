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
 * non-blocking uart interface
 *
 * @author Stefan Unterschuetz
 */

#include "uart_printf.h"
#include "printf.h"
#include "uart.h"

#define PRINTF_BUFFER	256


static uint8_t buffer[PRINTF_BUFFER];
uint16_t end = 0;
uint16_t begin = 0;
uint16_t length = 0;


static void putc_uart(char c) {
	if (length == PRINTF_BUFFER) {
		return; // queue is full
	} else {
		buffer[end] = c;
		end = (end + 1) % PRINTF_BUFFER;
		length++;
		if (!uart_txBusy()) {
			uart_tx(&buffer[begin], 1);
		}
	}

}

#ifdef __cplusplus
extern "C" {
#endif

void uart_txCallback(uint8_t *data, uint8_t bytesSent) {
	length--;
	begin = (begin + 1) % PRINTF_BUFFER;
	if (length) {
		uart_tx(&buffer[begin], 1);
	}
}

void uart_rxCallback(uint8_t *data, uint8_t received) {
}

#ifdef __cplusplus
}
#endif



void uart_printf_init(uint32_t baudrate) {
	uart_init(baudrate);
	printf_init(putc_uart);

}


