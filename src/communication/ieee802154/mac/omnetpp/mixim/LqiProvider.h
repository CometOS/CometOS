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

#ifndef LQIPROVIDER_H_
#define LQIPROVIDER_H_

#include "DeciderResult802154Narrow.h"
#include "mac_definitions.h"

namespace cometos {


/**
 * Abstract base for LQI-calculating objects. Note that for use with the
 * MacAbstractionLayer an implementation of this interface has to additionally
 * set itself as the LqiProvider.
 */
class LqiProvider {
public:
    virtual ~LqiProvider()
    {}

    /**
     * Calculates an LQI value based on the given decider result.
     * @param[in]  deciderResult decider result as propagated by the Phy layer
     * @param[out] isValid       set by the method to mark whether the returned
     *                           LQI value is valid
     * @return     calculated LQI value, only valid if isValid is true
     */
    virtual lqi_t calculateLqi(const DeciderResult802154Narrow & deciderResult, bool &isValid) = 0;
};

}

#endif /* LQIPROVIDER_H_ */
