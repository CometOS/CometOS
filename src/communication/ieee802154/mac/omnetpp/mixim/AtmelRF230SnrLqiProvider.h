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

#ifndef SNRLQIPROVIDER_H_
#define SNRLQIPROVIDER_H_

#include "LqiProvider.h"
#include "FWMath.h"
#include "SList.h"
#include "Module.h"

namespace cometos {

/**
 * Lqi provider for the Atmel RF23x family of radio transceivers
 * (including ATmegaXYRFA1,ATmegaXYRFR2 etc.).
 * Offers 9 discrete values which map an interval of SINR values to an LQI value.
 * The thresholds are derived from the LQI-PER curve given in the datasheet
 * of the ATmega128RFA1 on page 73.
 * Note that the actual PER may vary depending on the frame length.
 */
class AtmelRF230SnrLqiProvider : public cometos::Module, public LqiProvider {
    static const uint8_t numSteps = 8;
    struct Threshold {
        Threshold(lqi_t lqi = 0, double snr=0.0) :
            associatedLqi(lqi),
            snr(snr)
        {}

        lqi_t associatedLqi;
        double snr;
    };

public:
    virtual void initialize(int stage);

    StaticSList<Threshold, numSteps> thList;

    AtmelRF230SnrLqiProvider();

    virtual lqi_t calculateLqi(const DeciderResult802154Narrow & result, bool &isValid);
};


}

#endif /* SNRLQIPROVIDER_H_ */
