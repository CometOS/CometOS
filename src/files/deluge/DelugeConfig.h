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

#ifndef DELUGECONFIG_H
#define DELUGECONFIG_H


#define DELUGE_OUTPUT

/***
 * Value that defines the packet size
 */
#define DELUGE_PACKET_SEGMENT_SIZE      90

/***
 * Value that defines the page size
 */
#define DELUGE_PAGE_SIZE                2700

/**
 *
 */
#define DELUGE_PAGES_BEFORE_FLUSH         20

/**
 * value that defines how many packets are inside a page
 */
#define DELUGE_PACKETS_PER_PAGE         (DELUGE_PAGE_SIZE / DELUGE_PACKET_SEGMENT_SIZE)

/***
 * Name of the info file
 */
#define DELUGE_INFO_FILE                "info.deluge"

/***
 * Name of the data file
 */
#define DELUGE_DATA_FILE                "data.deluge"

/***
 * Defines out of range value for 8bit integer
 */
#define DELUGE_UINT8_OUT_OF_RAGE        255

/***
 * Defines how many hosts are stored inside RX
 */
#define DELUGE_RX_SUITABLE_HOSTS_SIZE   10

/***
 * Value that defines the maximum data file size
 */
#define DELUGE_MAX_DATAFILE_SIZE 130000

/***
 * Value that defines the maximum info file size
 */
#define DELUGE_MAX_INFOFILE_SIZE 768

/***
 *  Value that defines the segment size of the info file
 */
#define DELUGE_INFOFILE_SEGMENT_SIZE 32

/***
 * Value that defines the minimum round time in maintenance state
 */
#define DELUGE_MIN_ROUND_TIME 250

/***
 * Value that defines the maximum round time in maintenance state
 */
#define DELUGE_MAX_ROUND_TIME 1000

/***
 * Value that defines the limit of propagations during a round overheard by this node
 * If this the number of propagations with same versions received is higher than this value, the node will not propagate its summary
 */
#define DELUGE_VERSION_PROPAGATION_LIMIT 10

/***
 * Defines how long the DelugeHandler waits after the new file command is started
 */
#define DELUGE_HANDLER_DELAY 1000

/***
 * Defines the time RX waits until leaving RX state because of no receipt
 */
#define DELUGE_RX_NO_RECEIPT_DELAY 5000

/***
 * Defines the time TX waits between sending two packets
 */
#define DELUGE_TX_SEND_DELAY 100

#endif
