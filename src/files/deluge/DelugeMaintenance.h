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

#ifndef DELUGEMAINTENANCE_H
#define DELUGEMAINTENANCE_H

#include "cometosError.h"
#include "DelugeInfo.h"
#include "Callback.h"
#include "Arbiter.h"
#include "AirString.h"
#include "Endpoint.h"

namespace cometos {

class Deluge;

class DelugeMaintenance {
public:
    // Constructor
    DelugeMaintenance(Deluge *pDeluge);
    // Destructor
    ~DelugeMaintenance();
    // Handle a received summary
    void handleSummary(Airframe& frame, node_t source);
    // Handle a received object profile
    void handleObjectProfile(Airframe& frame);

    void timer();

private:
    // Called when a message is sent
    void onMessageSent(cometos_error_t result);
    // Scales the datafile to required size
    void scaleDatafile();
    // Sends our object profile
    void sendObjectProfile();
    // starts new round
    void newRound(time_ms_t roundDuration = 0);
    // Sends our summary
    void sendSummary();
    // Refresh RX
    void refreshRX(bool force = false);

private:
    Deluge* pDeluge = nullptr;

    // The number of propagations with same version overheard during current round
    uint16_t mSameVersionPropagationAmount = 0;

    // Informs that this round is already done
    bool mRoundProcessed = false;

    // The random value used to define the moment to send propagation in this round
    time_ms_t mRandomValue = 0;

    // The duration this round takes
    time_ms_t mRoundDuration = DELUGE_MIN_ROUND_TIME;

};

}

#endif
