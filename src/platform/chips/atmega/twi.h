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

#ifndef TWI_H_
#define TWI_H_

/*INCLUDES-------------------------------------------------------------------*/

#include <util/twi.h>
#include <stdint.h>
#include <stdbool.h>


/*PROTOTYPES-----------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/**Initializes twi interface.
 */
void twi_init();


/**Receives one byte from twi.
 *
 * @byte	pointer to store read data
 * @ack		if true ack is send to device, otherwise no ack is send
 * @return	status code
 *
 */
uint8_t twi_receive(uint8_t *byte, bool ack);


/**Sends byte to twi device.
 *
 * @byte	byte is transmitted to twi device
 * @return status code
 */
uint8_t twi_send(uint8_t byte);

/**
 * Sends start signal.
 * @return status code
 * */
uint8_t twi_start();


/**
 * Sends stop signal
 */
void twi_stop();

#ifdef __cplusplus
}
#endif

#endif /* TWI_H_ */
