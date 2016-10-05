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

#ifndef ATRF_HARDWARE_H

#include <avr/io.h>

// those used to be defined in avr headers, but this has changed from RFA1 to RFR2
// therefore, we define them here
#define P_ON                            0
#define BUSY_RX                         1
#define BUSY_TX                         2
#define RX_ON                           6
#define TRX_OFF                         8
#define PLL_ON                          9
#define SLEEP                           15
#define BUSY_RX_AACK                    17
#define BUSY_TX_ARET                    18
#define RX_AACK_ON                      22
#define TX_ARET_ON                      25
#define STATE_TRANSITION_IN_PROGRESS    31
#define TST_DISABLED                    0
#define TST_ENABLED                     1
#define CCA_BUSY                        0
#define CCA_IDLE                        1
#define CCA_NOT_FIN                     0
#define CCA_FIN                         1

#define CMD_NOP                         0
#define CMD_TX_START                    2
#define CMD_FORCE_TRX_OFF               3
#define CMD_FORCE_PLL_ON                4
#define CMD_RX_ON                       6
#define CMD_TRX_OFF                     8
#define CMD_PLL_ON                      9
#define CMD_RX_AACK_ON                  22
#define CMD_TX_ARET_ON                  25
#define TRAC_SUCCESS                    0
#define TRAC_SUCCESS_DATA_PENDING       1
#define TRAC_SUCCESS_WAIT_FOR_ACK       2
#define TRAC_CHANNEL_ACCESS_FAILURE     3
#define TRAC_NO_ACK                     5
#define TRAC_INVALID                    7

#define TX_PWR0                         0
#define TX_PWR1                         1
#define TX_PWR2                         2
#define TX_PWR3                         3
#define PA_LT0                          4
#define PA_LT1                          5
#define PA_BUF_LT0                      6
#define PA_BUF_LT1                      7

#define TX_PWR_MASK                     0x0F
#define CHANNEL_MASK                    0x1F
#define TRX_STATUS_MASK                 0x1F
#define RSSI_MASK                       0x1F

#define TRANSCEIVER_STATE (TRX_STATUS & TRX_STATUS_MASK)

#endif
