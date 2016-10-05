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

/*
 * @author Andreas Weigel
 */

#include "AtmelRF230SnrLqiProvider.h"
#include "MacAbstractionLayer.h"

namespace cometos {

Define_Module(AtmelRF230SnrLqiProvider);

AtmelRF230SnrLqiProvider::AtmelRF230SnrLqiProvider() {
    thList.push_back(Threshold(255, FWMath::dBm2mW( 2.5)));
    thList.push_back(Threshold(223, FWMath::dBm2mW( 0.7)));
    thList.push_back(Threshold(191, FWMath::dBm2mW( 0.1)));
    thList.push_back(Threshold(159, FWMath::dBm2mW(-1.1)));
    thList.push_back(Threshold(127, FWMath::dBm2mW(-1.8)));
    thList.push_back(Threshold(95,  FWMath::dBm2mW(-2.4)));
    thList.push_back(Threshold(63,  FWMath::dBm2mW(-3.0)));
    thList.push_back(Threshold(31,  FWMath::dBm2mW(-3.7)));
}

void AtmelRF230SnrLqiProvider::initialize(int stage) {
    if (stage == 1) {
        // set this LQI provider as the active one within the MAL
        cometos::MacAbstractionLayer * macPtr =
               (cometos::MacAbstractionLayer *) getModule(MAC_MODULE_NAME);
        if (macPtr != NULL) {
            macPtr->setLqiProvider(this);
        }
    }
}


lqi_t AtmelRF230SnrLqiProvider::calculateLqi(const DeciderResult802154Narrow & result, bool &isValid) {
    double snr = result.getSnr();
    isValid = true;
    for (uint8_t i = thList.begin(); i != thList.end(); i = thList.next(i)) {
        Threshold th = thList.get(i);
        if (snr > th.snr) {
            return th.associatedLqi;
        }
    }
    return 0;
}

}
