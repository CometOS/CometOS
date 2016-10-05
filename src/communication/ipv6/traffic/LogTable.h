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

#ifndef STATISTICS_H_
#define STATISTICS_H_

#include "stdint.h"




class NormLog {
public:

    static const uint16_t MAXVAL=65000;
    static const uint16_t STEPS=64;
    static const uint16_t STEPSIZE = (MAXVAL / STEPS);

    /**
     * Returns an approximation of ln(x/MAXVAL) * 1000.
     * Uses STEPS "exact" (minus rounding) tabled values and uses linear interpolation
     * between them.
     */
    static uint16_t logTimes1000(uint16_t x) {
        ASSERT(x < MAXVAL);
        uint16_t val = 0;
        uint16_t idx = x / STEPSIZE;

        val = LOGMAP[idx] + ((LOGMAP[idx+1] - LOGMAP[idx]) * (x % STEPSIZE)) / STEPSIZE;
        return val;
    }

private:
    static const uint16_t LOGMAP[STEPS] = { 11091, 10398,  9992,  9705,  9481,  9299,  9145,  9011,  8894,  8788,  8693,  8606,  8526,  8452,  8383,  8318 ,
            7657,  7262,  6980,  6760,  6580,  6427,  6295,  6178,  6074,  5979,  5892,  5813,  5739,  5670,  5606,  5546 ,
            4884,  4490,  4207,  3988,  3807,  3655,  3522,  3406,  3301,  3206,  3120,  3040,  2966,  2898,  2833,  2773 ,
            2112,  1717,  1435,  1215,  1035,   882,   750,   633,   528,   434,   347,   268,   194,   125,    61,     0
           };

};



#endif
