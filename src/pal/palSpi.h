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
 * CometOS Platform Abstraction Layer for Spi Communication.
 *
 * We tried to keep this interface as simple as possible by still
 * providing an solid base for a various number of application.
 * Three functions and two callbacks are totally used.
 *
 * The Spi driver has to support two internal queues, i.e.
 * rx and tx queue.  palSpi_write and  palSpi_read are the
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
 */
#ifndef PALSPI_H_
#define PALSPI_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "cometosError.h"
#include "IGPIOPin.h"
#include "Arbiter.h"

// TODO interface should be completely refactored to using the class-template-based approach
//      aribters should be returned based on pointers of the SPI instance.
//      For ATmega, one SPI instance (i.e., no different classes) was enough, for other platforms
//      there are multiple SPI modules usable, so we need to adapt here

//enum {
//	PAL_SPI_SUCCESS = 1,
//	PAL_SPI_NOT_MASTER = 2,
//	PAL_SPI_ERROR = 3,
//	PAL_SPI_BUSY = 4,
//	PAL_SPI_NOT_SUPPORTED = 5
//};
//typedef uint8_t palSpi_ret_t;

/** Four different modes are allowed:
 * 0: CPOL=0, CPHA=0 --> sample on leading(rising), setup on trailing(falling)
 * 1: CPOL=0, CPHA=1 --> setup on leading(rising), sample on trailing(falling)
 * 2: CPOL=1, CPHA=0 --> sample on leading(falling), setup on trailing(rising)
 * 3: CPOL=1, CPHA=1 --> setup on leading(falling), sample on trailing(rising)*/
typedef uint8_t palSpi_mode_t;


typedef void (*palSpi_cbPtr) (const uint8_t * txBuf, uint8_t * rxBuf, uint16_t len, cometos_error_t result);

/**Initializes SPI
 *
 * @param asMaster if true, SPI will be initialized as master
 *                 if false, SPI will be initialied as slave
 *                 palSpi_transmit is only available in master mode
 * @param freq     speed of SPI interface in Hertz, ignored if asMaster is false
 * @param mode     defines the SPI mode to be used, see palSpi_mode_t
 * @param lsbFirst if set to true, the least significant bit of the current
 *                 byte is shifted out first, if false, the most significant
 * @return PAL_SPI_SUCCESS if spi was initialized successfully
 *         PAL_SPI_NOT_SUPPORTED if tried to init in master mode and speed is not supported
 *         PAL_SPI_ERROR in case of some other error
 */
cometos_error_t palSpi_init(bool asMaster, uint32_t freq, palSpi_mode_t mode, bool lsbFirst);


/**Enables chip select signal to indicate the begin of a transmission. Only
 * has an effect if SPI was initialized as master before.
 */
void palSpi_enable();

/**Disables chip select signal to indicate the end of an transmission. Only
 * has an effect if SPI was initialized as master before.
 */
void palSpi_disable();

/**
 * NOTE: this function does NOT change the chip select signal, this has to be
 * done by the user via palSpi_enable()/palSpi_disable() (or other GPIOs)!
 * Write one byte of data to the SPI interface. Blocks until the transmission
 * is complete. In master mode, the byte will be sent to the slave as soon as
 * possible, if no other SPI transmission is ongoing. In slave mode, this
 * operation puts the given byte into the data register and blocks until
 * the master initiates a transmission. The received byte is written
 * into given rx one byte buffer.
 *
 * @param  tx   byte to be sent to the SPI
 * @param  rx   pointer to byte rcvd from the other end
 *
 * @return PAL_SPI_SUCCESS    if transmission was successful
 *         PAL_SPI_ERROR      if some other error occurred
 */
cometos_error_t palSpi_swapByte(uint8_t tx, uint8_t * rx);



/**
 * NOTE: this function does NOT change the chip select signal, this has to be
 * done by the user via palSpi_enable()/palSpi_disable() (or other GPIOs)!
 * Write a buffer to the SPI interface. In master mode, the buffer
 * is written to the SPI as soon as possible, if it is not busy. Received bytes
 * are stored in the given rx buffer.
 *
 * In slave mode, the first byte of the buffer is directly put into the
 * SPI data register. As soon as the master initiates a transmission,
 * it is sent and the next byte is put into the register until all bytes
 * are sent.
 *
 * This function returns after initiating the transmission. After
 * len bytes have been successfully transmitted, the given callback
 * function is called.
 *
 * @param  txBuf   buffer to be sent to the SPI; may be NULL if dummy data should
 *                 be sent instead (NOTE: one of rxBuf or txBuf MUST be non-null)
 * @param  rxBuf   buffer to store bytes received from the other end
 *                 may be NULL, if received bytes can be discarded
 * @param  len     length of the data to transmit and receive
 * @param  callback pointer to a callback function which is called when
 *                  the transmission is finished
 *
 * @return PAL_SPI_SUCCESS    if transmission was successfully initiated
 * 		   PAL_SPI_BUSY       if the SPI is currently busy
 *         PAL_SPI_ERROR      if some other error occurred
 */
cometos_error_t palSpi_transmit(const uint8_t * txBuf, uint8_t * rxBuf, uint16_t len, palSpi_cbPtr callback);

/**
 * NOTE: this function does NOT change the chip select signal, this has to be
 * done by the user via palSpi_enable()/palSpi_disable() (or other GPIOs)!
 *
 * Similar to palSpi_transmit, but will block until len bytes are transmitted.
 *
 * @param  txBuf   buffer to be sent to the SPI; may be NULL if dummy data should
 *                 be sent instead (NOTE: one of rxBuf or txBuf MUST be non-null)
 * @param  rxBuf   buffer to store bytes received from the other end
 *                 if this is a NULL pointer, all received bytes are
 *                 discarded
 * @param  len     length of the data to transmit and receive
 *
 * @return PAL_SPI_SUCCESS    if transmission was successfully initiated
 * 		   PAL_SPI_BUSY       if the SPI is currently busy
 *         PAL_SPI_ERROR      if some other error occurred
 */
cometos_error_t palSpi_transmitBlocking(const uint8_t * txBuf, uint8_t * rxBuf, uint16_t len);


cometos::Arbiter* getSPIArbiter();

namespace cometos {


/**
 * A abstract definition of an SPI class. This can be used as alternative to
 * the functions defined above, which makes sense for MCUs that have more than
 * one SPI (eg. STM32).
 */
class PalSpi {
public:
	/**
	 * Initialize SPI controller with given parameters
	 *
	 * @param asMaster Defines, if the MCU is the bus master
	 * @param freq 	The baudrate of the SPI Bus
	 * @param mode Mode (sample on rising/falling edge etc.)
	 * @param lsb Defines the order of the bits in the communication
	 */
	virtual cometos_error_t init(bool asMaster, uint32_t freq, palSpi_mode_t mode, bool lsbFirst) = 0;

	/**
	 * Enables the spi transfer by setting the chipSelect bit, if ChipSelect is active High,
	 * or clearing the ChipSelect bit if chipSelect is active low.
	 *
	 * Asserts, if the spi was already enabled before the call.
	 */
	virtual void enable() = 0;

	/**
	 * Disable the spi transfer by clearing the chipSelect bit, if ChipSelect is active High,
	 * or setting the ChipSelect bit if chipSelect is active low.
	 *
	 */
	virtual void disable() = 0;

	/**
	 * Sets the pin that should be used as chip select pin.
	 * @param pin Pointer to the IGPIOPin Object representing the pin. The
	 * 		pin will be initialized by this function to be disabled.
	 *
	 * @param active_low Specifies, if the pin is active-low or active high.
	 */
	virtual void setCsPin(IGPIOPin* pin, bool active_low) = 0;

	/**
	 * Transmit and receive a single byte
	 */
	virtual cometos_error_t swapByte(uint8_t tx, uint8_t *rx) = 0;

	/**
	 * Transmit a number of bytes asynchronously. Calls the given callback, if the transmission is done.
	 * @param txBuf Base address of the buffer to read transmit bytes from
	 * @param rxBuf Base address of the buffer to write received bytes to
	 * @param len Number of bytes to exchange.
	 * @param callback Callback to call, when transmission is done
	 */
	virtual cometos_error_t transmit(const uint8_t *txBuf, uint8_t *rxBuf, uint16_t len, palSpi_cbPtr callback) = 0;

	/**
	 * Transmits a number of bytes synchronously.
	 *
	 * @param txBuf Base address of the buffer to read transmit bytes from
	 * @param rxBuf Base address of the buffer to write received bytes to
	 * @param len Number of bytes to exchange.
	 *
	 */
	virtual cometos_error_t transmitBlocking(const uint8_t *txBuf, uint8_t* rxBuf, uint16_t len) = 0;

	/**
	 * @return True, if no transmission is active at the moment, false else.
	 */
	virtual bool isAvailable() = 0;

	/**
	 * Returns an instance of this class
	 *
	 * @param port The number of the SPI port
	 */
	template<typename PortType>
	static PalSpi* getInstance(PortType port);

protected:
	PalSpi() {}
};

}

#endif /* PALSPI_H_ */
