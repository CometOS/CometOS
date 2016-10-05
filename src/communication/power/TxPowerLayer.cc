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
 */

#include "TxPowerLayer.h"
#include "MacControl.h"
#include "mac_interface.h"
#include "ParameterStore.h"
#include "logging.h"

namespace cometos {

void TxPower::doSerialize(ByteVector& buf) const {
    serialize(buf, pwrLvl);
}
void TxPower::doUnserialize(ByteVector& buf) {
    unserialize(buf, pwrLvl);
}

Define_Module(TxPowerLayer);

TxPowerLayer::TxPowerLayer(const char * service_name, uint8_t initialPwr):
        Layer(service_name),
        txp(initialPwr)
{}

TxPowerLayer::~TxPowerLayer() {
}


void TxPowerLayer::initialize() {
    Layer::initialize();
    remoteDeclare(&TxPowerLayer::setPwrLvl, "spl");
    remoteDeclare(&TxPowerLayer::getPwrLvl, "gpl");
    remoteDeclare(&TxPowerLayer::resetPwrLvl, "rpl");
    remoteDeclare(&TxPowerLayer::setAutomaticDiversityState, "div");
    remoteDeclare(&TxPowerLayer::setRxLnaState, "lna");
    LOG_DEBUG("");
}

void TxPowerLayer::handleRequest(DataRequest *msg) {
    if (!msg->has<MacControl>()) {
        msg->set(new MacControl(txp.pwrLvl));
    } else {
        msg->get<MacControl>()->txPower = txp.pwrLvl;
    }
    LOG_DEBUG("Set power lvl: " << (int) txp.pwrLvl);
    gateReqOut.send(msg);
}


void TxPowerLayer::handleIndication(DataIndication * msg) {
    gateIndOut.send(msg);
}

TxPower TxPowerLayer::getPwrLvl() {
    cometos::ParameterStore* ps = cometos::ParameterStore::get(*this);
    TxPower storedCfg = txp;
    if (ps != NULL) {
        ps->getCfgData(this, storedCfg);
    } else {
        storedCfg.setPersistent(false);
    }
    storedCfg.setEqualToActiveConfig(storedCfg == txp);

    return storedCfg;
}

cometos_error_t TxPowerLayer::setPwrLvl(TxPower& newPwrLvl) {
    if (newPwrLvl.isValid()) {
        cometos::ParameterStore* ps = cometos::ParameterStore::get(*this);
        if (ps != NULL) {
            ps->setCfgData(this, newPwrLvl);
        } else {

        }
        this->txp = newPwrLvl;
        return COMETOS_SUCCESS;
    } else {
        return COMETOS_ERROR_INVALID;
    }
}

cometos_error_t TxPowerLayer::resetPwrLvl() {
    cometos_error_t result = COMETOS_SUCCESS;
    cometos::ParameterStore* ps = cometos::ParameterStore::get(*this);
    if (ps != NULL) {
        result = ps->resetCfgData(this);
    }

    return result;
}


void TxPowerLayer::setAutomaticDiversityState(uint8_t& enable, uint8_t& selected_antenna) {
    mac_disableAutomaticDiversity(selected_antenna); // disable to set selected antenna
    if(enable) {
    	mac_enableAutomaticDiversity();
    }
}

void TxPowerLayer::setRxLnaState(uint8_t& enable) {
    mac_setRxLnaState(enable);
}

} // namespace cometos
