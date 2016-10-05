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

#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stdbool.h>

/*PROTOTYPES-----------------------------------------------------------------*/


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Initializes UART.
 *
 * @param baud_rate used baud rate (e.g. 57600)
 */
void uart_init(uint32_t baudrate);


/**
 * @return true, if UART tries to receives data, otherwise false
 */
bool uart_rxBusy();


/**
 * @return true, if UART tries to send data, otherwise false
 */
bool uart_txBusy();


/**
 * Tries to receive the given number of bytes. Call is non-blocking.
 * After data is received or uart_rxAbort is called, the
 * function uart_rxCallback is invoked.
 *
 * @param data		buffer for storing received data
 * @param length	size of data array
 */
bool uart_rx(uint8_t *data, uint8_t length);


/**
 * Tries to send the given number of bytes. Call is non-blocking.
 * After data is sent or uart_txAbort is called, the
 * function uart_txCallback is invoked.
 *
 * @param data		buffer containing data for sending
 * @param length	size of data array
 */
bool uart_tx(uint8_t *data, uint8_t length);


/**
 * Immediately stops ongoing receptions and calls uart_rxCallback. The latter is inhibited
 * if currently no transmission is ongoing.
 */
void uart_rxAbort();


/**
 * Immediately stops ongoing transmissions and calls uart_txCallback. The latter is inhibited
 * if currently no transmission is ongoing.
 *
 */
void uart_txAbort();


/**
 * @param data 			buffer containing received data
 * @param bytesReceived	number of received bytes
 */
void uart_rxCallback(uint8_t *data, uint8_t bytesReceived);


/**
 * @param data 		buffer containing sent data
 * @param bytesSent	number of sent bytes
 */
void uart_txCallback(uint8_t *data, uint8_t bytesSent);


#ifdef __cplusplus
}
#endif


#endif // UART_H

