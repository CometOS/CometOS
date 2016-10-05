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

#ifndef LINKLAYERINFORMATION_H_
#define LINKLAYERINFORMATION_H_

#include "MacAbstractionBase.h"

namespace cometos_v6 {


class LlRxInfo : public cometos::Object {
public:
    LlRxInfo() {
        clear();
    }

    LlRxInfo(const cometos::MacRxInfo * const info) {
        sumRssi = info->rssi;
        sumLqi = info->lqi;
        lqiValid = info->lqiIsValid;
        n = 1;
    }

    void clear() {
        sumRssi = 0;
        sumLqi = 0;
        lqiValid = true;
        n = false;
    }

    virtual LlRxInfo * getCopy() const {
        LlRxInfo * tmp = new LlRxInfo();
        *tmp = *this;
        return tmp;
    }

    void newFrame(mac_dbm_t rssi, lqi_t lqi, bool lqiValid) {
        sumRssi += rssi;
        sumLqi += lqi;
        n++;
        if (this->lqiValid) {
            this->lqiValid = lqiValid;
        }
    }

    mac_dbm_t avgRssi() const {
        return sumRssi / n;
    }

    bool isLqiValid()  const {
        return lqiValid && isValid();
    }

    lqi_t avgLqi()  const {
        return sumLqi / n;
    }

    bool isValid()  const {
        return n > 0;
    }

private:
    int16_t sumRssi;
    uint16_t sumLqi;
    bool lqiValid;
    uint8_t n;
};




class LlTxInfo : public cometos::Object {
public:
    LlTxInfo(uint8_t numRetries = 0, uint8_t numSuccessfulTransmissions = 0) :
        numRetries(numRetries),
        numSuccessfulTransmissions(numSuccessfulTransmissions)
    {}

    void reset() {
        numRetries = 0;
        numSuccessfulTransmissions = 0;
    }

    void setFailed() {
        numSuccessfulTransmissions &= ~SUCCESS_FLAG;
    }

    void setSuccess() {
        numSuccessfulTransmissions |= SUCCESS_FLAG;
    }

    bool increaseNumTransmissions(uint8_t by) {
        if (numSuccessfulTransmissions < SUCCESS_FLAG - 1) {
            numSuccessfulTransmissions++;
            return true;
        } else {
            return false;
        }
    }

    void increaseNumRetries(uint8_t by) {
        numRetries += by;
    }

    uint8_t getNumSuccessfulTransmissions() const {
        return numSuccessfulTransmissions & ~SUCCESS_FLAG;
    }

    uint8_t getNumRetries() const {
        return numRetries;
    }

    bool success() const {
        return numSuccessfulTransmissions & SUCCESS_FLAG;
    }

    virtual cometos::Object* getCopy() const {
        return new LlTxInfo(this->numRetries,
                              this->numSuccessfulTransmissions);
    }


private:
    static const uint8_t SUCCESS_FLAG = 0x80;

    uint8_t numRetries;       /* how many fragments the packet was split into */
    uint8_t numSuccessfulTransmissions; /* how many total link transmissions were required */
};

}


#endif /* LINKLAYERINFORMATION_H_ */
