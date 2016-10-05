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

#ifndef MAC_INTERFACE_EXT_H
#define MAC_INTERFACE_EXT_H

#include "rf231.h"

/**
 * This functions sets the device, with which all other functions in the
 * implementation of mac_interface.h will work from here on.
 */
void mac_setRadioDevice(cometos::Rf231 *device );

/* This functions notifies the RFA1DriverLayer, that it should set the TxPower
 * to a new value before sending next time
 */
void radio_setTxPowerLvl(mac_power_t lvl);

/* This functions notifies the RFA1DriverLayer, that it should set the CCAMode
 * to a new value before performing CCA next time
 */
void radio_setCCAMode(mac_ccaMode_t ccaMode);

/* This functions notifies the RFA1DriverLayer, that it should set the Threshold
 * to a new value before performing CCA next time
 */
void radio_setCCAThreshold(mac_dbm_t ccaThreshold);

void radio_setNetworkId(mac_networkId_t id);

mac_networkId_t radio_getNetworkId();

void radio_setNodeId(mac_nodeId_t addr);

mac_nodeId_t radio_getNodeId();

#endif //MAC_INTERFACE_EXT_H
