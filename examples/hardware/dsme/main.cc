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

/*INCLUDES-------------------------------------------------------------------*/

#include "cometos.h"
#include "TrafficEvaluation.h"
#include "palLed.h"
#include "palId.h"
#include "OutputStream.h"
#include "logging.h"
#include "SList.h"

#ifdef DSME
#include "DSMEPlatform.h"
#endif

#include "CsmaMac.h"

#include "palPin.h"

/*PROTOTYPES-----------------------------------------------------------------*/

using namespace cometos;

int main() {
    TrafficEvaluation* traffic;

    palId_init();

    getCout() << "Booting " << hex << palId_id() << dec << endl;

    /* instantiate traffic modules */
    traffic = new TrafficEvaluation(60, 500);

    /* set up destination */
    if (palId_id() != PAN_COORDINATOR) {
        traffic->setDestination(PAN_COORDINATOR);
    }

#ifdef DSME
    dsme::DSMEPlatform mac("mac");
    //mac.setLogLevel(LOG_LEVEL_INFO);
#else
    CsmaMac mac("mac");
#endif

    //   if (palId_id() != PAN_COORDINATOR) {
    /* connect gates */
    traffic->gateReqOut.connectTo(mac.gateReqIn);
    mac.gateIndOut.connectTo(traffic->gateIndIn);

    traffic->setLogLevel(LOG_LEVEL_DEBUG);

    /* customizing CometOS's logging facility */
    //cometos::setRootLogLevel(LOG_LEVEL_ERROR);
    cometos::setRootLogLevel(LOG_LEVEL_INFO);

    //getCout() << "Booted" << endl;

    /* start system */
    cometos::initialize();

    getCout() << "Booted " << hex << palId_id() << dec << endl;

    cometos::run();
    return 0;
}
