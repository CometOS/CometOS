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
#ifndef EMPIRICDECIDERBASE_H_
#define EMPIRICDECIDERBASE_H_

#include <BaseDecider.h>
#include <PhyToMacControlInfo.h>
#include "types.h"

/**
 * Abstract class. Serves as a basis for empirical deciders.
 * A derived empirical Decider has to implement the
 * getEmpiricAttenuationMapping(AirframePtr frame), providing an attenuation
 * mapping for the signal of the given air frame. E.g. to set the received
 * signal power to a certain level, one could simply divide the target mapping
 * by the current signal power mapping and return the result as attenuation
 * mapping.
 *
 * As the attenuation mapping is added to the signal within the
 * processSignal(AirframePtr frame) method, this method MUST NOT be
 * overridden by a deriving class.
 */
class EmpiricDeciderBase : public BaseDecider {
public:
	EmpiricDeciderBase(DeciderToPhyInterface* phy,
			   double sensitivity,
			   int myIndex,
			   bool debug)
		: BaseDecider(phy, sensitivity, myIndex, debug),
		  numDroppedWithInterference(0),
		  numDroppedWithoutInterference(0),
		  numFramesWithInterference(0),
		  numFramesWithoutInterference(0),
		  numFramesReceived(0),
		  numFramesMissed(0)
	{};

protected:
	/**
	 * DO NOT OVERRIDE this method in any derived class.
	 * It makes sure that the empiric attenuation mapping
	 * is added to the incoming signal ONCE when the AirFrame
	 * is first passed from the PhyLayer. If overwritten, this
	 * has to be done be the derived class!
	 */
	virtual simtime_t processSignal(AirFrame* frame);

	/**
	 * Derived empirical deciders have to override this method to return
	 * the attenuation mapping for the incoming frame. This mapping is then
	 * applied once to the signal within the processSignal method (which is
	 * why processSignal() must not be overwritten).
	 */
	virtual Mapping* getEmpiricalAttenuationMapping(AirFrame* frame) = 0;


	// members for statistics collection
	/** number of frames dropped due to collisions */
	unsigned long numDroppedWithInterference;

	/** number of frames dropped because of bit errors (FCS failed) */
	unsigned long numDroppedWithoutInterference;

	/** number of frames which suffered from interference */
	unsigned long numFramesWithInterference;

	/** number of frames which were received without interference */
	unsigned long numFramesWithoutInterference;

	/** number of frames successfully received */
	unsigned long numFramesReceived;

	/** number of times the signal was to weak to receive the preamble/SFD */
	unsigned long numFramesMissed;

private:

};

#endif /* EMPIRICDECIDERBASE_H_ */
