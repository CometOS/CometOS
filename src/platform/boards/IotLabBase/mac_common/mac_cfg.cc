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
 * @author Andreas Weigel
 * @author Peter Oppermann
 */

#include "mac_interface.h"
#include "mac_interface_ext.h"

static cometos::Rf231* rf;

/** maximum CCA threshold of this transceiver */
static const mac_dbm_t CCA_TH_MAX = -60;

/** minimum CCA threshold of this transceiver */
static const mac_dbm_t CCA_TH_MIN = -90;

void mac_setRadioDevice(cometos::Rf231* device){
	rf = device;
}

// Not safe when the RFA1DriverLayer is running in parallel
// -> Use the function provided there!
mac_result_t mac_setChannel(mac_channel_t channel) {
	if (MAC_MIN_CHANNEL > channel || MAC_MAX_CHANNEL < channel){
		return MAC_ERROR_FAIL;
	}

	uint8_t reg = rf->readRegister(AT86RF231_REG_PHY_CC_CCA);
	reg = (~AT86RF231_PHY_CC_CCA_MASK_CHANNEL & reg ) | channel;
	rf->writeRegister(AT86RF231_REG_PHY_CC_CCA, reg);

	return MAC_SUCCESS;
}

mac_channel_t mac_getChannel() {
	uint8_t reg = rf->readRegister(AT86RF231_REG_PHY_CC_CCA);
	return reg & AT86RF231_PHY_CC_CCA_MASK_CHANNEL;
}

mac_result_t mac_setNetworkId(mac_networkId_t id) {
	if (id == MAC_NWK_BROADCAST){
		return MAC_ERROR_FAIL;
	}
	radio_setNetworkId(id);
	return MAC_SUCCESS;
}

mac_networkId_t mac_getNetworkId() {
	return radio_getNetworkId();
}

mac_result_t mac_setNodeId(mac_nodeId_t addr) {

	if (addr == MAC_BROADCAST) {
		return MAC_ERROR_FAIL;
	}
	radio_setNodeId(addr);

	return MAC_SUCCESS;
}

mac_nodeId_t mac_getNodeId() {
	/*because of long SPI delays I have chosen to save the node id
	 * 	separately on the microcontroller insted reading it over
	 * 	the spi each time
	 */
	return radio_getNodeId();
}


mac_result_t mac_setCCAMode(mac_ccaMode_t mode) {
	if (mode <= MAC_CCA_MODE_ENERGY_AND_CS) {
		radio_setCCAMode(mode);
		return MAC_SUCCESS;
	}
	else {
		return MAC_ERROR_INVALID;
	}
}

mac_ccaMode_t mac_getCCAMode() {
	uint8_t reg = rf->readRegister(AT86RF231_REG_PHY_CC_CCA);
	reg &= AT86RF231_PHY_CC_CCA_MASK_CCA_MODE;

	return reg >> AT86RF231_PHY_CC_CCA_MODE0;
}

mac_dbm_t mac_getCCAThreshold() {
	uint8_t reg = rf->readRegister(AT86RF231_REG_CCA_THRES);
	return CCA_TH_MIN + ((reg & 0x0f) * 2);
}

mac_result_t mac_setCCAThreshold(mac_dbm_t threshold) {
	if (threshold >= CCA_TH_MIN && threshold <= CCA_TH_MAX) {
		// set threshold accordingly
		uint8_t ccaThresholdValue = (threshold - CCA_TH_MIN) / 2;
		// reset threshold bits
		uint8_t reg = rf->readRegister(AT86RF231_REG_CCA_THRES);
		reg &= ~0x0F;
		//Set new value
		reg |= ccaThresholdValue;
		rf->writeRegister(AT86RF231_REG_CCA_THRES, reg );
		return MAC_SUCCESS;
	} else {
		return MAC_ERROR_SIZE;
	}
}


mac_result_t mac_setTxPower(mac_power_t pwrLevel){
	if (pwrLevel > mac_getMaxTxPowerLvl()
			|| pwrLevel < mac_getMinTxPowerLvl()){
		return COMETOS_ERROR_INVALID;
	}
	radio_setTxPowerLvl(pwrLevel);

	return COMETOS_SUCCESS;
}


mac_power_t mac_getMinTxPowerLvl(){
	return 0;
}

mac_power_t mac_getMaxTxPowerLvl(){
	return 0x0F;
}

mac_result_t mac_on() {
    //TODO
    return MAC_SUCCESS;
}

mac_result_t mac_sleep() {
    //TODO
    return MAC_SUCCESS;
}

mac_result_t mac_setRxLnaState(bool enable) {
    return MAC_SUCCESS;
}

mac_result_t mac_enableAutomaticDiversity() {
    return MAC_SUCCESS;
}

mac_result_t mac_disableAutomaticDiversity(uint8_t selected_antenna) {
    return MAC_SUCCESS;
}


