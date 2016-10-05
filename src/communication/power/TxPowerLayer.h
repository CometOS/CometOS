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

#ifndef TXPOWERLAYER_H_
#define TXPOWERLAYER_H_

#include "Layer.h"
#include "PersistableConfig.h"
#define TX_POWER_LAYER_MODULE_NAME "txp"

namespace cometos {

struct TxPower : public PersistableConfig {
    TxPower(uint8_t pwrLvl = 0xFF) :
        pwrLvl(pwrLvl)
    {}

    bool operator==(const TxPower& rhs) {
        return pwrLvl == rhs.pwrLvl;
    }

    bool isValid() {
        return true;
    }

    virtual void doSerialize(ByteVector& buf) const;
    virtual void doUnserialize(ByteVector& buf);
    uint8_t pwrLvl;
};



class TxPowerLayer : public Layer {
public:
    TxPowerLayer(const char * service_name = TX_POWER_LAYER_MODULE_NAME, uint8_t initialPwr = 0xFF);

    virtual void initialize();

    virtual void handleRequest(DataRequest * msg);

    virtual void handleIndication(DataIndication * msg);

    virtual ~TxPowerLayer();

    TxPower getPwrLvl();

    cometos_error_t resetPwrLvl();

    cometos_error_t setPwrLvl(TxPower & newPwrLvl);

    void setAutomaticDiversityState(uint8_t& enable, uint8_t& selected_antenna);
    void setRxLnaState(uint8_t& enable);

private:
    TxPower txp;
};



} // namespace cometos

#endif /* TXPOWERLAYER_H_ */
