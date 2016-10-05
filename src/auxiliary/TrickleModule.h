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

#ifndef TRICKLE_H_
#define TRICKLE_H_

#include "LongScheduleModule.h"

namespace cometos {

class TrickleModule;
/**
 * Implementation of the Trickle algorithm
 * (http://tools.ietf.org/html/rfc6206#section-4.1)
 * for the CometOS. This implementation supports milli second accuracy
 * and a maximum interval of 2^32 ms (~49 days).
 *
 * From RFC 6206:
 * Imin is defined in units of time (e.g., milliseconds, seconds)
 * IMax is defined as a number of interval doublings
 * k is the redundancy constant (Trickle suppresses transmissions if it received
 *   more than k consistent transmissions already when the timer fires)
 *
 * Some confusion may arise with regard to section 4.2 of RFC 6206:
 * It says, at start, Trickle sets I to a number in [IMin, IMax]. To our
 * understanding, this makes no sense and should be written
 * "Trickle set I to a number in [IMin, 2^IMax * IMin]" or "in [1, IMax]" and
 * then use [(2^I)/2, I^2) as interval for the real time span.
 *
 * This implementation follows our understanding of the algorithm.
 */
class TrickleModule : public LongScheduleModule {
public:

    TrickleModule(const char * service_name, uint16_t iMin, uint8_t iMax, uint8_t k);

    virtual ~TrickleModule();

    virtual void initialize();

    virtual void finish();

    /**
     * Has to be called by the client every time a Trickle-relevant
     * transmission is received. If the reception of an inconsistent
     * transmission is communicated to Trickle, it will rest itself.
     *
     * @param isInconsistent indicates whether the received transmission is
     *        consistent with the current state.
     */
    void transmissionReceived_(bool isInconsistent);

    /**
     * Starts Trickle as described in the RFC (chooses random interval
     * between min and max). This method is not called automatically in
     * initialize.
     */
    bool start_();

    /**
     * Cancels operation of Trickle. After this method has been called,
     * transmit won't fire until start_ or reset_ are called.
     */
    bool stop_();

    /**
     * Similar to start, but resets Trickle to use the minimum interval instead
     * of choosing a random value between min and max intervals. Can be used
     * if some Trickle-external event should cause Trickle to reset.
     */
    void reset_();

    /**
     * Checks if the Trickle timers are active. Trickle won't call transmit
     * in inactive state.
     *
     * @return true, if timers are running and Trickle algorithm is executed
     *         false, if timers are idle and Trickle algorithm is no executed
     */
    bool isActive();

    /**
     * Change parameters the Trickle algorithm.  If Trickle is currently active,
     * it will stop, change the parameters  and start again. If it is
     * not active, the parameters will simply be changed and Trickle remains
     * inactive.
     */
    void setTrickleModule(uint16_t iMin, uint8_t iMax, uint8_t k);

private:
    static const uint8_t iMin = 0;

    void txTimeout_(LongScheduleMsg * msg);

    void intervalTimeout_(LongScheduleMsg * msg);

    void startInterval_();

    void cancelTimers();

    virtual void transmit() = 0;

	uint16_t iUnit;
	uint8_t iMax;
	uint8_t k;

	uint8_t iCurr;
	uint8_t c;

	LongScheduleMsg txTimer;
	LongScheduleMsg intervalTimer;

#ifdef OMNETPP
	omnetpp::SimTime last;
	omnetpp::cOutVector sendVector;
#endif
};


} /* namespace cometos */
#endif /* ASSOCIATIONSERVICE_H_ */
