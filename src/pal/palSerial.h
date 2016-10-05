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
 * CometOS Platform Abstraction Layer for Serial Communication.
 *
 * We tried to keep this interface as simple as possible by still
 * providing an solid base for a various number of application.
 * Three functions and two callbacks are totally used.
 *
 * The serial driver has to support two internal queues, i.e.
 * rx and tx queue.  palSerial_write and  palSerial_read are the
 * thread-safe functions to work with these queues. Both functions
 * are not blocking and return the number of successfully read/written
 * bytes. Blocking operations can be implemented on base of these
 * functions.
 *
 * Length of buffers can be defined by by platform implementation.
 * However, forCometOS we require a minimum length of 128 Byte for input
 * and output.
 *
 *
 * @author Stefan Unterschuetz
 * @author Florian Meier (as class and 9 bit support)
 */
#ifndef PALSERIAL_H_
#define PALSERIAL_H_

#include <stdint.h>
#include "Task.h"
#include "Callback.h"

namespace cometos {

class PalSerial {
public:
	/**Initializes UART and empties internal queues. Re-initialization
	 * during runtime must be possible.
	 *
	 * Callbacks (may be executed from interrupt or other thread):
	 * <li> rxStartCallback: called after empty rx queue is filled with first byte
	 * <li> txFifoEmptyCallbaclk called after tx queue becomes empty
	 * <li> txReadyCallbaclk called after last byte was sent
	 *
	 * @param baudrate baudrate of serial connection (maximum allowed 57600)
	 * @param rxStartCallback NULL or task to execute as callback
	 * @param txFifoEmptyCallback NULL or task to execute as callback
	 * @param txReadyCallback NULL or task to execute as callback
	 */
	virtual void init(uint32_t baudrate, Task* rxStartCallback,
	        Task* txFifoEmptyCallback, Task* txReadyCallback) = 0;

	/**Enables or disables 9 bit mode. The 9th bit signals the start of a frame with an address.
	 *
     * @param enable 9 bit mode if true
     * @param enable multi processor mode if true
	 * @param rxByteCallback Callback to execute for each byte.
	 * 			 First parameter is the byte value, second parameter the flag (9th bit).
	 * 			 Return value is false if all bytes until the next one with enabled flag are to be discarded.
 	 */
	virtual void set_9bit_mode(bool enable, bool multiProcessorMode, Callback<bool(uint8_t,bool)> rxByteCallback) = 0;

	/**Writes data into tx buffer. Is non-blocking and thread-safe.
	 * @param data Data to be written
	 * @param length Length of the data
	 * @param flagFirst Set the 9th bit at the first byte
 	 * @return number of written bytes
 	 */
	virtual uint8_t write(const uint8_t* data, uint8_t length, bool flagFirst = true) = 0;


	/**Reads data from rx buffer. Is non-blocking and thread-safe.
 	 * @return number of read bytes
 	 */
	virtual uint8_t read(uint8_t* data, uint8_t length) = 0;


	/**Returns an instance of PalSerial.
	 * @param port if platform supports multiple ports, these can be selected with this parameter
 	 * @return PalSerial instance matching the given port
 	 */
	template<typename PortType>
	static PalSerial* getInstance(PortType port);

protected:
	PalSerial() {}
};

}

#endif /* PALSERIAL_H_ */

