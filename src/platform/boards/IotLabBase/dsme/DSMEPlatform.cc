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

#include "DSMEPlatform.h"
#include "dsme_settings.h"
#include "RFA1DriverLayer.h"
#include "palId.h"
#include "helper/DSMEMessageBuffer.h"
#include "openDSME/dsmeLayer/DSMELayer.h"
#include "MacSymbolCounter.h"
#include "rf231.h"
#include "mac_interface_ext.h"

#ifndef PAN_COORDINATOR
#define PAN_COORDINATOR false
#endif

using namespace cometos;

namespace dsme {

DSMEPlatform* DSMEPlatform::instance = nullptr;

/* all zero, this all gets handled by MAC-layer */
mac_ackCfg_t DSMEPlatform::ackCfg { 0, 0 };
mac_backoffCfg_t DSMEPlatform::backoffCfg { 0, 0, 0, 0 };

DSMEPlatform::DSMEPlatform(const char * name) :
        DSMEPlatformBase(name),
        /* TODO remove
        gateReqIn(this, &DSMEPlatform::handleRequest, "gateReqIn"),
        gateIndOut(this, "gateIndOut"),
        phy_pib(10),
        mac_pib(phy_pib),

        mcps(dsmeLayer),
        mlme(dsmeLayer),

        dsmeAdaptionLayer(dsmeLayer),

        */
        channel(MIN_CHANNEL) {
    instance = this;
}

void DSMEPlatform::initialize() {
    auto rf = Rf231::getInstance();
    mac_setRadioDevice(rf);

    DSMEPlatformBase::initialize();

    this->dsme->setPHY_PIB(&(this->phy_pib));
    this->dsme->setMAC_PIB(&(this->mac_pib));
    this->dsme->setMCPS(&(this->mcps_sap));
    this->dsme->setMLME(&(this->mlme_sap));

    this->dsmeAdaptionLayer.initialize();

    /* Initialize Address */
    IEEE802154MacAddress address;

    uint16_t id = palId_id();
    translateMacAddress(id, this->mac_pib.macExtendedAddress);

    timer = new Message();
    ccaTimer = new Message();

    this->mac_pib.macShortAddress = this->mac_pib.macExtendedAddress.getShortAddress();
    if((PAN_COORDINATOR == palId_id())) {
        this->mac_pib.macPANId = MAC_DEFAULT_NWK_ID;
    }

    this->mac_pib.macAssociatedPANCoord = (PAN_COORDINATOR == palId_id());
    this->mac_pib.macBeaconOrder = 6;
    this->mac_pib.macSuperframeOrder = 3;
    this->mac_pib.macMultiSuperframeOrder = 5;

    this->mac_pib.macMinBE = 3;
    this->mac_pib.macMaxBE = 8;
    this->mac_pib.macMaxCSMABackoffs = 5;
    this->mac_pib.macMaxFrameRetries = 3;

    this->mac_pib.macDSMEGTSExpirationTime = 7;
    this->mac_pib.macResponseWaitTime = 16;

    this->mac_pib.recalculateDependentProperties();

    settings->isPANCoordinator = this->mac_pib.macAssociatedPANCoord;
    settings->isCoordinator = settings->isPANCoordinator;

    this->phy_pib.phyCurrentChannel = 11;
    settings->optimizations = false;

    this->dsmeAdaptionLayer.settings.allocationScheme = DSMEAdaptionLayerSettings::ALLOC_CONTIGUOUS_SLOT;

    this->dsmeAdaptionLayer.setIndicationCallback(DELEGATE(&DSMEPlatform::handleDataMessageFromMCPS, *this));
    this->dsmeAdaptionLayer.setConfirmCallback(DELEGATE(&DSMEPlatform::handleConfirmFromMCPS, *this));

    mac_result_t result = RFA1Driver_init(this->mac_pib.macShortAddress, 0, this->channel, 0x00, &DSMEPlatform::ackCfg,
            &DSMEPlatform::backoffCfg);
    ASSERT(result == MAC_SUCCESS);

    // Enable MacSymbolCounter
    MacSymbolCounter::getInstance().init(CALLBACK_MET(&DSMEEventDispatcher::timerInterrupt, dsme::DSMEPlatform::instance->getDSME()->getEventDispatcher()));
	uint8_t trx_ctrl_1 = rf->readRegister(AT86RF231_REG_TRX_CTRL_1);
	trx_ctrl_1 |= AT86RF231_TRX_CTRL_1_MASK_IRQ_2_EXT_EN;
	rf->writeRegister(AT86RF231_REG_TRX_CTRL_1, trx_ctrl_1);

#ifdef RFA1_ENABLE_EXT_ANT_SW
#error "Antenna diversity does not work if capture is used"
#endif

}

}
