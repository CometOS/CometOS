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
#ifndef DECIDER_RESULT_EMPIRIC_802154_H_
#define DECIDER_RESULT_EMPIRIC_802154_H_

#include <DeciderResult802154Narrow.h>

/**
 * @brief Based on MiXiM's DeciderResult802154Narrow.
 *
 * Adds information about the interference status of a frame reception, i.e.,
 * whether the frame was received/dropped with interference from other
 * frames or without.
 */
class DeciderResultEmpiric802154 : public DeciderResult802154Narrow {
public:

	/**
	 * @brief Initialises with the passed values.
	 */
	DeciderResultEmpiric802154(bool isCorrect,
	                           double bitrate,
	                           double snr,
	                           double ber,
	                           double rssi,
	                           bool interferenceDuringRx,
	                           bool failTxAlready,
	                           bool failTxDuringRx):
		DeciderResult802154Narrow(isCorrect, bitrate, snr, ber, rssi),
		interferenceDuringRx(interferenceDuringRx),
		failTxAlready(failTxAlready),
		failTxDuringRx(failTxDuringRx)
	{}

	bool hadInterference() const {
	    return interferenceDuringRx;
	}

	bool getFailTxAlready() const {
	    return failTxAlready;
	}

	bool getFailTxDuringRx() const {
	    return failTxDuringRx;
	}

private:
	bool interferenceDuringRx;
	bool failTxAlready;
	bool failTxDuringRx;
};

#endif /* DECIDERRESULT80211_H_ */
