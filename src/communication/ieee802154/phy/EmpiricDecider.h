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
#ifndef EMPIRICDECIDER_H
#define EMPIRICDECIDER_H

#include <BaseDecider.h>
#include <PhyToMacControlInfo.h>
#include "EmpiricDeciderBase.h"
#include "types.h"


/**
 * stores statistics for a certain link, containing rssi, its variation
 * and the packet error rate
 */
class LinkStats
{
protected:
	double rssiMean;
	double rssiVar;
	double per;

public:
	void setRssiMean(double value) {
		rssiMean = value;
	}
	void setRssiVar(double value) {
		rssiVar = value;
	}
	void setPer(double value) {
		per = value;
	}
	double getRssiMean() {
		return rssiMean;
	}
	double getRssiVar() {
		return rssiVar;
	}
	double getPer() {
		return per;
	}
};

typedef std::map<node_t, LinkStats> LinkVector;

/**
 * ONLY WORKS CORRECTLY FOR TXPOWER = 0dBm at the moment (because it
 * simply uses the configured received signal power values as attenuation).
 * Implements a Decider using pre-configured data sets to determine
 * signal strength of incoming packets. For this purpose it overwrites
 * the getEmpiricAttenuationMapping method of EmpiricDeciderBase.
 *
 * TODO: Refactor to work with newer MiXiM versions
 * TODO: Add a header signal processing and apply address filtering - this
 *       could help a MAC layer to not wait needlessly for ongoing receptions
 *       which are useless.
 */
class EmpiricDecider : public EmpiricDeciderBase {
private:
	LinkVector linkVector;
	int sfdLen;
//	double currentSignalPwr;

public:
	EmpiricDecider(DeciderToPhyInterface* phy,
				       LinkVector linkVector,
				       int headerLength,
				       double snrThreshold,
				       double sensitivity,
				       int myIndex,
				       double interferenceCorrFactor,
				       bool debug);

protected:

	/**
	 * @inheritDoc
	 */
	virtual simtime_t processNewSignal(AirFrame* frame);


	/**
	 * @inheritDoc
	 */
	virtual simtime_t processSignalEnd(AirFrame* frame);

	/**
	 * @inheritDoc
	 */
	virtual Mapping* getEmpiricalAttenuationMapping(AirFrame* frame);


	void finish();

private:
	double interferenceCorrectionFactor;
	double const BER_LOWER_BOUND;
	static double const RSSI_DBM_UPPER_BOUND;

	Mapping* createConstantMapping(simtime_t start, simtime_t end, double value);

	/**
	 * Get the probability of a bit error from a bit-energy to noise ratio
	 */
	double getBERFromSNR(double snr);

	double n_choose_k(int n, int k);

	bool syncOnSFD(AirFrame* frame, bool & intermittentTxState);

	double evalBER(AirFrame* frame, bool & intermittentTxState);

	LinkStats getLinkStatsForFrame(AirFrame *frame);

	void sendControl(omnetpp::cPacket *frame, int kind, bool hasInterference, bool failTxAlready, bool failTxDuringRx);

	bool checkInterferenceStatus(simtime_t start, simtime_t end);

	void printMapping(ConstMapping* m);
	std::string toString(simtime_t v, unsigned int length);
	template<class T> std::string toString(T v, unsigned int length);
	double toDecibel(double v);

};

#endif /* EMPIRICDECIDERTEST_H_ */
