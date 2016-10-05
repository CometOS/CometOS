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

#include "cometos.h"
#include "palLed.h"
#include "palLocalTime.h"
#include "Module.h"
#include "adc.h"

using namespace cometos;


/*PROTOTYPES-----------------------------------------------------------------*/

class TestADC : public cometos::Module {
public:

    virtual void initialize() {
        adc_init();
        schedule(new cometos::Message(), &TestADC::testADC, 200);
        schedule(new cometos::Message(), &TestADC::testPrintPulse, 3200);
    }

    void testPrintPulse(cometos::Message* msg) {
        if (pulseCount > 5) {
            noPulseCount = 0;
            if (pulseCount > 10) {
                schedule(msg, &TestADC::testPrintPulse, 1800);
            } else {
                schedule(msg, &TestADC::testPrintPulse, 18000/pulseCount);
            }
            uint16_t thisBPM = ((pulseCount - 2) * 600000) / (pulseTime - highesPulseTime - lowestPulseTime);
            if (bpm == 0) {
                bpm = thisBPM;
            } else {
                bpm = (bpm + thisBPM * 3) / 4;
            }
            cometos::getCout() << "Pulse of " << (bpm/10) << "." << (int)(bpm%10) << "bpm" << cometos::endl;
            pulseCount = 0;
            pulseTime = 0;
            highesPulseTime = 0;
            lowestPulseTime = 0;
        } else if (pulseCount == 0 || noPulseCount > 11) {
            schedule(msg, &TestADC::testPrintPulse, 8000);
            cometos::getCout() << "NO PULSE!" << cometos::endl;
            pulseCount = 0;
            noPulseCount = 0;
        } else {
            schedule(msg, &TestADC::testPrintPulse, 1000);
            noPulseCount++;
        }
    }

    void testADC(cometos::Message* msg) {
        schedule(msg, &TestADC::testADC, 20);
        uint16_t val = adc_readAvgWOExtrema(ADC0_CH, 5);

        if (rising && val >= maxVal) {
            maxVal = val;
            countAnomalie = 0;
        } else if (!rising && val <= minVal) {
            minVal = val;
            countAnomalie = 0;
        } else if (countAnomalie < 4) {
            countAnomalie++;
        } else {
            threshold = (minVal + maxVal) / 2;
            if (rising) {
                rising = false;
                minVal = val;
            } else {
                rising = true;
                maxVal = val;
            }
        }
        if (rising && val > ((threshold + maxVal*2) / 3) && countAnomalie == 0) {
            if (!counted) {
                pulseCount++;
                uint16_t thisPulseTime = (palLocalTime_get() - lastPulseTime);
                pulseTime += thisPulseTime;
                lastPulseTime = palLocalTime_get();
                if (highesPulseTime < thisPulseTime) {
                    highesPulseTime = thisPulseTime;
                }
                if (lowestPulseTime == 0 || lowestPulseTime > thisPulseTime) {
                    lowestPulseTime = thisPulseTime;
                }
                counted = true;
                palLed_toggle(1);
            }
        } else if (!rising && val < ((threshold + minVal*2) / 3)  && countAnomalie == 0) {
            if (counted) {
                palLed_toggle(1);
                counted = false;
            }
        }
    }

    uint16_t maxVal = 0;
    uint16_t minVal = 0xFFFF;
    uint16_t threshold = 0;


    uint8_t pulseCount = 0;
    uint16_t bpm = 0;
    uint16_t pulseTime = 0;
    uint16_t highesPulseTime = 0;
    uint16_t lowestPulseTime = 0;
    uint32_t lastPulseTime = 0;

    bool rising = false;
    uint8_t countAnomalie = 0;
    bool counted = false;

    uint8_t noPulseCount = 0;
};

TestADC tadc;

int main() {
    cometos::initialize();
    palLed_on(2);

    cometos::getCout() << "Start" << cometos::endl;

    cometos::run();
    return 0;
}


