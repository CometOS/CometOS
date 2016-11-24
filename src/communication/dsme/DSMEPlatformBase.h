/*
 * openDSME
 *
 * Implementation of the Deterministic & Synchronous Multi-channel Extension (DSME)
 * described in the IEEE 802.15.4-2015 standard
 *
 * Authors: Florian Meier <florian.meier@tuhh.de>
 *          Maximilian Koestler <maximilian.koestler@tuhh.de>
 *          Sandrina Backhauss <sandrina.backhauss@tuhh.de>
 *
 * Based on
 *          DSME Implementation for the INET Framework
 *          Tobias Luebkert <tobias.luebkert@tuhh.de>
 *
 * Copyright (c) 2015, Institute of Telematics, Hamburg University of Technology
 * All rights reserved.
 *
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

#ifndef DSMEPLATFORMBASE_H
#define DSMEPLATFORMBASE_H

#include <stdint.h>
#include <stdlib.h>

#include "DSMEMessage.h"
#include "openDSME/dsmeAdaptionLayer/DSMEAdaptionLayer.h"
#include "openDSME/helper/DSMEDelegate.h"
#include "openDSME/interfaces/IDSMEPlatform.h"
#include "openDSME/mac_services/dataStructures/IEEE802154MacAddress.h"
#include "openDSME/mac_services/mcps_sap/MCPS_SAP.h"
#include "openDSME/mac_services/mlme_sap/MLME_SAP.h"
#include "openDSME/mac_services/pib/MAC_PIB.h"
#include "openDSME/mac_services/pib/PHY_PIB.h"
#include "dsme_settings.h"
#include "Airframe.h"

#include "MacAbstractionLayer.h"
#include "DSMEMessageBuffer.h"
#include "HandleMessageTask.h"

namespace dsme {

class DSMESettings;
class DSMELayer;
class DSMEAdaptionLayer;

class DSMEPlatformBase : public cometos::MacAbstractionLayer, public IDSMEPlatform {
public:

    DSMEPlatformBase(const char* service_name = NULL);
    virtual ~DSMEPlatformBase();

    /* cometos::MacAbstractionLayer */
    virtual void initialize() override;

    virtual void rxEnd(cometos::Airframe *frame, node_t src, node_t dst, cometos::MacRxInfo const & info) override;

    virtual void txEnd(cometos::macTxResult_t result, cometos::MacTxInfo const & info) override;

    virtual void handleRequest(cometos::DataRequest* req);

    cometos::InputGate<cometos::DataRequest> gateReqIn;

    /* IDSMEPlatformBase */

    bool isReceptionFromAckLayerPossible() override;

    void handleReceivedMessageFromAckLayer(DSMEMessage* message) override;

    void setReceiveDelegate(receive_delegate_t receiveDelegate) override;

    void updateVisual() override {};

    virtual uint16_t getRandom() override {
        return intrand(UINT16_MAX);
    }

    IEEE802154MacAddress& getAddress() {
        return this->mac_pib.macExtendedAddress;
    }

    DSMEMessage* getEmptyMessage() override;

    void releaseMessage(DSMEMessage* msg) override;

    void scheduleStartOfCFP();

    void start();

    void printSequenceChartInfo(DSMEMessage* msg);

protected:
    virtual void signalNewMsg(DSMEMessage* msg) {}
    virtual void signalReleasedMsg(DSMEMessage* msg) {}

    void handleDataMessageFromMCPS(DSMEMessage* msg);
    void handleConfirmFromMCPS(DSMEMessage* msg, DataStatus::Data_Status dataStatus);

    bool send(cometos::Airframe* frame);

    DSMEMessage* getLoadedMessage(cometos::Airframe* frame);

    PHY_PIB phy_pib;
    MAC_PIB mac_pib;

    DSMELayer* dsme;

    mcps_sap::MCPS_SAP mcps_sap;
    mlme_sap::MLME_SAP mlme_sap;

    DSMEAdaptionLayer dsmeAdaptionLayer;

    uint16_t messagesInUse;

    receive_delegate_t receiveFromAckLayerDelegate;

    cometos::Message* timer;
    cometos::Message* ccaTimer;
    Delegate<void(bool)> txEndCallback;

    DSMESettings* settings;

    DSMEMessageBuffer messageBuffer;

    /** @brief the bit rate at which we transmit */
    double bitrate;

    void printDSMEManagement(uint8_t management, DSMESABSpecification& sabSpec, CommandFrameIdentifier cmd);

    bool channelInactive;

    /** @brief Copy constructor is not allowed.
     */
    DSMEPlatformBase(const DSMEPlatformBase&);
    /** @brief Assignment operator is not allowed.
     */
    DSMEPlatformBase& operator=(const DSMEPlatformBase&);

    void translateMacAddress(node_t& from, IEEE802154MacAddress& to);

    cometos::CallbackTask startTask;
    HandleMessageTask handleMessageTask;
    cometos::CallbackTask handleStartOfCFPTask;
};

}

#endif
