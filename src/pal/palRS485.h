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
 * CometOS Platform Abstraction Layer for Serial Communication over RS485.
 *
 * @author Florian Meier
 */
#ifndef PALRS485_H_
#define PALRS485_H_

#include <stdint.h>
#include "Task.h"
#include "Callback.h"

namespace cometos {

/**Initializes RS485.
 *
 * Callbacks (may be executed from interrupt or other thread):
 * <li> rxStartCallback: called after empty rx queue is filled with first byte
 * <li> txFifoEmptyCallbaclk called after tx queue becomes empty
 * <li> txReadyCallbaclk called after last byte was sent
 *
 * @param if platform supports multiple port, thes can be selected with this parameter
 * @param baudrate baudrate of serial connection (maximum allowed 57600)
 * @param rxStartCallback NULL or pointer to callback function
 * @param txFifoEmptyCallback NULL or pointer to callback function
 * @param txReadyCallback NULL or pointer to callback function
 */
void palRS485_init(uint32_t baudrate, Task* rxStartCallback,
		Task* txFifoEmptyCallback, Task* txReadyCallback);

/**Writes data into tx buffer. Is non-blocking and thread-safe.
 * @param data Data to be written
 * @param length Length of the data
 * @param flagFirst Set the 9th bit at the first byte
 * @return number of written bytes
 */
uint8_t palRS485_write(const uint8_t* data, uint8_t length, bool flagFirst = true);

/**Reads data from rx buffer. Is non-blocking and thread-safe.
 * @return number of read bytes
 */
uint8_t palRS485_read(uint8_t* data, uint8_t length);

/**Enables or disables 9 bit mode. The 9th bit signals the start of a frame with an address.
 *
 * @param enable 9 bit mode if true
 * @param enable multi processor mode if true
 * @param rxByteCallback Callback to execute for each byte.
 * 			 First parameter is the byte value, second parameter the flag (9th bit).
 * 			 Return value is false if all bytes until the next one with enabled flag are to be discarded.
 */
void palRS485_9bit_mode(bool enable, bool multiProcessorMode, Callback<bool(uint8_t,bool)> rxByteCallback);

}

#endif /* PALRS485_H_ */
