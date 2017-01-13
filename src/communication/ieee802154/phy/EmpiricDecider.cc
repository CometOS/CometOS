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
#include <EmpiricDecider.h>
#include "DeciderResultEmpiric802154.h"
#include "MacAbstractionLayer.h"
#include "MacPacket.h"

using namespace omnetpp;

#define ST 50
#define DEBUG(x)
//{std::cout<< "EmpiricDecider|"<<simTime().dbl()<<" "<< x;}


double const EmpiricDecider::RSSI_DBM_UPPER_BOUND = -40;

EmpiricDecider::EmpiricDecider(DeciderToPhyInterface* phy,
                               LinkVector linkVector,
                               int headerLength,
                               double sfdLen,
                               double sensitivity,
                               int myIndex,
                               double interferenceCorrFactor,
                               bool debug)
        : EmpiricDeciderBase(phy, sensitivity, myIndex, debug),
          linkVector(linkVector),
          sfdLen(sfdLen),
//          currentSignalPwr(0.0),
          interferenceCorrectionFactor(interferenceCorrFactor),
          BER_LOWER_BOUND(1e-8)
    {
//        std::cout << "sfdLen=" << this->sfdLen << std::endl;
    };



simtime_t EmpiricDecider::processNewSignal(AirFrame* frame) {
    EV << "Processing new signal." << endl;
    DEBUG("processing new signal " << std::endl);

    Signal& s = frame->getSignal();
    simtime_t start = s.getSignalStart();
    Mapping* mbr = s.getBitrate();
    double rate = mbr->getValue(Argument(start));

    simtime_t end = start + (sfdLen / rate);

    bool hasInterference = checkInterferenceStatus(start, end);

    //check if we are not currently trying to receive another signal
    if(currentSignal.first != 0) {
        EV << "We are already trying to receive a signal, so this is noise." << endl;

        // count other incoming frames which cannot be received
        // because some other signal is already being received as
        // "dropped because of interference", too? I'd say: yes!
        sendControl(frame, BaseDecider::PACKET_DROPPED, true, false, false);
        numDroppedWithInterference++;
        numFramesWithInterference++;
        numFramesMissed++;
        return notAgain;
    }

    bool intermittentTxState = false;
    if (!syncOnSFD(frame, intermittentTxState)) {
        // we count a radio TX state during a reception as interference here
        sendControl(frame, BaseDecider::PACKET_DROPPED, hasInterference, intermittentTxState, false);
        numDroppedWithInterference++;
        numFramesWithInterference++;
        numFramesMissed++;
        return notAgain;
    }

    DEBUG("processing new frame " << std::endl);


    EV << "Signal is strong enough to receive." << endl;

    //set currently receiving signal to this signal
    currentSignal.first = frame;
    currentSignal.second = EXPECT_END;

    //we consider the channel now busy
    setChannelIdleStatus(false);

    DEBUG("Filtering Signal for " << frame->getDuration() << " s" << endl;);

    return start + frame->getDuration();
}


//uint8_t dummy() {
//    uint8_t blub = 1;
//    uint8_t sum = blub + blub;
//    return sum;
//}

simtime_t EmpiricDecider::processSignalEnd(AirFrame* frame) {
    assert(frame == currentSignal.first);
    assert(currentSignal.second == EXPECT_END);

    Signal& signal = frame->getSignal();
    simtime_t start = signal.getSignalStart();
    simtime_t end = start + signal.getSignalLength();

    ASSERT(phy->getSimTime() == end);
    DEBUG("Processing end of signal" << endl;);
    // because final signal strength is reached only "eps" after start
    // and drops to zero after "eps" before end
    simtime_t receivingStart = MappingUtils::post(start);
    simtime_t receivingEnd = MappingUtils::pre(phy->getSimTime());

    EV << "Processing end of signal." << endl;

    bool hasInterference = checkInterferenceStatus(start, end);
    // did this airframe suffer from interference from other frames?
    if(hasInterference) {
        numFramesWithInterference++;
//        dummy();
    } else {
        numFramesWithoutInterference++;
    }

    //get accumulated noise strength
    Mapping* noise = calculateRSSIMapping(receivingStart, receivingEnd, frame);


    //get signal strength
    ConstMapping* rcvPower = frame->getSignal().getReceivingPower();
//  if (simTime().dbl() > ST) {
//      printMapping(rcvPower);
//      printMapping(rcvPower);
//  }

    // overall received signal strength
    ConstMapping* totalRSS = MappingUtils::add(*rcvPower, *noise);


    //calculate SINR
    Mapping* sinr = MappingUtils::divide(*rcvPower, *noise);

    //find smallest SNR value -- no longer necessary
//    double minSINR = MappingUtils::findMin(*sinr, Argument(receivingStart), Argument(receivingEnd));

    // to store average energy detected during reception
    double rssi = 0;


    bool noErrors = true;
    double ber = 0;
    double avgBer = 0;
    double errorProbability;
    double bitrate = signal.getBitrate()->getValue(start);

    ConstMappingIterator* it = sinr->createConstIterator(Argument(receivingStart));
    simtime_t currTime = it->getPosition().getTime();
    simtime_t duration;

    double minSINR = -1.0;
    bool intermittentTxState = false;
    while (currTime < receivingEnd) {
        // determine end of this interval
        simtime_t nextTime = receivingEnd;  //either the end of the signal...
        if(it->hasNext()) {     //or the position of the next entry
            nextTime = std::min(it->getNextPosition().getTime(), nextTime);
            it->next(); //the iterator will already point to the next entry
        }

        if (noErrors) {
            //get signal strength and SINR for this interval
            double channelEnergy = totalRSS->getValue(it->getPosition());
            double currSINR = it->getValue();
//            std::cout << "currSINR=" << currSINR << std::endl;

            // we reduce the SINR by a configurable number of dB here to artificially introduce more
            // bit errors for interfering frames, which otherwise have quite a good
            // chance (~27% for 95 byte frame) to arrive for an SINR of 0 dB. This
            // should account for varying signal strengths which are perceived in
            // reality (and are not reflected by our model which uses constant
            // signal strengths)
//            double snrdb_orig = FWMath::mW2dBm(snr);
            currSINR = currSINR * pow(10, interferenceCorrectionFactor / 10);
//            double snrdb_corr = FWMath::mW2dBm(snr);

            // update minimum SINR
            if (minSINR == -1.0 || minSINR > currSINR) {
                minSINR = currSINR;
            }

            // if we have a SINR of 0, this MUST mean that the radio was
            // switched from RX to some other mode
            if (currSINR == 0.0) {
                intermittentTxState = true;
            }
            duration = nextTime - currTime;

            // calculate number of bits during interval
            int numBits = int (duration.dbl() * bitrate);


            ber = getBERFromSNR(currSINR);
            avgBer += ber * numBits;
            rssi += channelEnergy * duration.dbl();

            errorProbability = 1 - pow((1-ber), numBits);
            noErrors = (errorProbability < phy->getModule()->uniform(0,1)) && (!intermittentTxState);

            DEBUG("ber=" << ber << "|numBits=" << numBits << std::endl);
        }

        currTime = nextTime;
    }

    rssi = rssi / (receivingEnd - receivingStart);
    avgBer = avgBer / frame->getBitLength();

    // clean up
    delete it;
    delete totalRSS;
    delete noise;
    delete sinr;

    // if no bit errors were detected:
    if(noErrors) {
//      std::cout << "phyRssi=" << rssi << "|toDbm=" << FWMath::mW2dBm(rssi) << endl;
        EV << "Signal received correctly. Sending to upper layer" << endl;
        numFramesReceived++;
        phy->sendUp(frame, new DeciderResultEmpiric802154(
                              true,
                              signal.getBitrate()->getValue(Argument(start)),
                              minSINR,
                              avgBer,
                              FWMath::mW2dBm(rssi),
                              hasInterference,
                              false,
                              false));

    } else {
        EV << "Signal could not be received correctly. Discarding signal." << endl;
        //send dropped packet up as control message
        sendControl(frame, BaseDecider::PACKET_DROPPED, hasInterference, false, intermittentTxState);

        // NOTE: the following may lead to inconsistent MAC vs PHY drops
        // only count packet as dropped here if the radio did not switch
        // to TX state during the reception of the frame, which may happen if
        // the SFD was received during the backoff/CCA phase of the TX
        // operation
        if (!intermittentTxState) {
            if(hasInterference) {
                numDroppedWithInterference++;
            } else {
                numDroppedWithoutInterference++;
            }
        }
    }

    currentSignal.first = 0;

    //the channel is now back idle
    setChannelIdleStatus(true);

    return notAgain;
}


Mapping* EmpiricDecider::getEmpiricalAttenuationMapping(AirFrame* frame) {
    LinkStats stats = getLinkStatsForFrame(frame);
    double recvPwr = (1 * phy->getModule()->normal(stats.getRssiMean(), sqrt(stats.getRssiVar())));
    if (recvPwr > RSSI_DBM_UPPER_BOUND) {
        recvPwr = RSSI_DBM_UPPER_BOUND;
    }

    simtime_t start = frame->getSignal().getSignalStart();
    simtime_t end = start + frame->getSignal().getSignalLength();

    // create mapping from targeted receive signal power
    // and return it as attenuation mapping (only works for txPwr = 0dBm)
    Mapping* empiricMapping = createConstantMapping(start, end, FWMath::dBm2mW(recvPwr));
//  printMapping(empiricMapping);
    return empiricMapping;
}



Mapping* EmpiricDecider::createConstantMapping(simtime_t start, simtime_t end,
        double value) {
    // create an empty mapping
    Mapping* resultMap = MappingUtils::createMapping(0.0,
            DimensionSet::timeDomain);
    Argument startPos(start);
    Argument endPos(end);
    resultMap->appendValue(start, value);
    resultMap->appendValue(endPos, value);
    return resultMap;
}


LinkStats EmpiricDecider::getLinkStatsForFrame(AirFrame* frame) {
    LinkStats stats;
    cometos::checked_ptr<cometos::MacPacket> mac(check_and_cast<cometos::MacPacket*> (frame->getEncapsulatedPacket()));
    if (!mac->meta.has<cometos::NodeId>()) {
        ASSERT(false);
    }
    const cometos::NodeId * src = mac->meta.get<cometos::NodeId>();

    ASSERT(src != NULL);
    if (linkVector.find(src->value) == linkVector.end()) {
        stats.setPer(1.0);
        stats.setRssiMean(-200.0);
        stats.setRssiVar(0.0);
        linkVector[src->value] = stats;
        EV << "No connection entry found, adding new disconnected one" << endl;
    }
    stats = linkVector[src->value];

    return stats;
}


void EmpiricDecider::finish() {
    phy->recordScalar("numDroppedWithInterference", numDroppedWithInterference);
    phy->recordScalar("numDroppedWithoutInterference", numDroppedWithoutInterference);
    phy->recordScalar("numFramesWithInterference", numFramesWithInterference);
    phy->recordScalar("numFramesWithoutInterference", numFramesWithoutInterference);
    phy->recordScalar("numFramesReceived", numFramesReceived);
    phy->recordScalar("numFramesMissed", numFramesMissed);
}

double EmpiricDecider::n_choose_k(int n, int k) {
    if (n < k)
        return 0.0;

    const int       iK     = (k<<1) > n ? n-k : k;
    const double    dNSubK = (n-iK);
    register int    i      = 1;
    register double dRes   = i > iK ? 1.0 : (dNSubK+i);

    for (++i; i <= iK; ++i) {
        dRes *= dNSubK+i;
        dRes /= i;
    }
    return dRes;
}

bool EmpiricDecider::syncOnSFD(AirFrame* frame, bool & intermittentTxState) {
    double BER;
    double sfdErrorProbability;

    BER = evalBER(frame, intermittentTxState);
    sfdErrorProbability = 1.0 - pow((1.0 - BER), sfdLen);

    bool success = sfdErrorProbability < phy->getModule()->uniform(0, 1, 0);
    return success;
}

double EmpiricDecider::evalBER(AirFrame* frame, bool & intermittentTxState) {
    Signal&       signal     = frame->getSignal();
    simtime_t     time       = MappingUtils::post(phy->getSimTime());
    Argument      argStart(time);
    double        rcvPower   = signal.getReceivingPower()->getValue(argStart);

    ConstMapping* noise      = calculateRSSIMapping(time, time, frame);
    double        noiseLevel = noise->getValue(argStart);

    // we can only determine if our radio is in TX state by checking for recv
    // power == 0.0; this comes from the RadioStateAnalogueModel
    if (rcvPower == 0.0) {
        intermittentTxState = true;
    } else {
        intermittentTxState = false;
    }
    double        ber        = getBERFromSNR(rcvPower/noiseLevel); //std::max(0.5 * exp(-rcvPower / (2 * noiseLevel)), DEFAULT_BER_LOWER_BOUND);

    delete noise;

    return ber;
}

double EmpiricDecider::getBERFromSNR(double snr) {
    double ber;


    // as long as RadioStateAnalougueModel is used with a attenuation of
    // 1.0 (resulting in snr=0.0) for radio states != RX,
    // we here check for snr=0.0 to return a
    // guaranteed bit error and not the 0.5 which result when calculating.
    // note that an SNR of 0 can only be caused by this RadioStateAnalougeModel
    // (this value is NOT in dB!), because all other attenuations have some
    // value > 0 --- the case that this happens by inaccuracies of double
    // by some extinction (and thereby drop regular packets which theoretically
    // have a chance for reception) is supposed to be negligible
    if (snr == 0.0) {
        return 1.0;
    }

    //if (simTime().dbl()) {
    //    DEBUG("snr=" << snr << "|snrdb_orig=" << snrdb_orig <<"|snrdb_corr=" << snrdb_corr << std::endl);
    //}

    // valid for IEEE 802.15.4 2.45 GHz OQPSK modulation
    // Following formula is defined in IEEE 802.15.4 standard, refer to the
    // 2006 standard, page 268, section E.4.1.8 Bit error rate (BER)
    // calculations, formula 7). Here you can see that the factor of 20.0 is correct ;).
    const double dSNRFct = 20.0 * snr;
    double       dSumK   = 0;
    int k       = 2;

    // following loops are optimized by using n_choose_k symmetries
    // n_choose_k(16, k) == n_choose_k(16, 16-k)
    for (; k < 8; k += 2) {
        // k will be 2, 4, 6 (symmetric values: 14, 12, 10)
        dSumK += n_choose_k(16, k) * (exp(dSNRFct * (1.0 / k - 1.0)) + exp(dSNRFct * (1.0 / (16 - k) - 1.0)));
    }
    // for k =  8 (which does not have a symmetric value)
    k = 8; dSumK += n_choose_k(16, k) * exp(dSNRFct * (1.0 / k - 1.0));
    for (k = 3; k < 8; k += 2) {
        // k will be 3, 5, 7 (symmetric values: 13, 11, 9)
        dSumK -= n_choose_k(16, k) * (exp(dSNRFct * (1.0 / k - 1.0)) + exp(dSNRFct * (1.0 / (16 - k) - 1.0)));
    }
    // for k = 15 (because of missing k=1 value)
    k   = 15; dSumK -= n_choose_k(16, k) * exp(dSNRFct * (1.0 / k - 1.0));
    // for k = 16 (because of missing k=0 value)
    k   = 16; dSumK += n_choose_k(16, k) * exp(dSNRFct * (1.0 / k - 1.0));
    ber = (8.0 / 15) * (1.0 / 16) * dSumK;
    // bit error probability for FSK modulation
    return std::max(ber, BER_LOWER_BOUND);
}

void EmpiricDecider::sendControl(omnetpp::cPacket *frame, int kind, bool hasInterference, bool failTxAlready, bool failTxDuringRx) {
    omnetpp::cMessage* mac = frame->decapsulate();
    ASSERT(mac != NULL);
    mac->setKind(kind);
    mac->setControlInfo(new PhyToMacControlInfo(new DeciderResultEmpiric802154(false,
                                                            0.0,
                                                            0.0,
                                                            0.0,
                                                            RSSI_INVALID,
                                                            hasInterference,
                                                            failTxAlready,
                                                            failTxDuringRx)));
    phy->sendControlMsg(mac);
}


bool EmpiricDecider::checkInterferenceStatus(simtime_t start, simtime_t end) {
    AirFrameVector channel;
    phy->getChannelInfo(start, end, channel);
    bool hasInterference = channel.size() > 1;
    return hasInterference;
}

///////////////////////////////////////////////////////////////////////////////
// quick and dirty mapping print methods  /////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

///**
// * @brief Quick and ugly printing of a two dimensional mapping.
// */
void EmpiricDecider::printMapping(ConstMapping* m) {
    Dimension frequency("frequency");
    //const DimensionSet& dims = m->getDimensionSet();

    std::set<simtime_t> timeEntries;
    std::set<double> freqEntries;

    std::map<double, std::set<simtime_t> > entries;

    ConstMappingIterator* it = m->createConstIterator();

    while (it->inRange()) {
        entries[it->getPosition().getArgValue(frequency)].insert(
                it->getPosition().getTime());
        timeEntries.insert(it->getPosition().getTime());
        freqEntries.insert(it->getPosition().getArgValue(frequency));

        if (!it->hasNext())
            break;

        it->next();
    }

    delete it;

    std::cout << "--------+---------------------------------------------------------"
            << endl;
    std::cout << "GHz \\ms | ";
    for (std::set<simtime_t>::const_iterator tIt = timeEntries.begin(); tIt
            != timeEntries.end(); ++tIt) {
        std::cout << toString(*tIt * 1000, 6) << " ";
    }
    std::cout << endl;
    std::cout << "--------+---------------------------------------------------------"
            << endl;
    if (freqEntries.begin() == freqEntries.end()) {
        std::cout << "        | Defines no own key entries." << endl;
        std::cout << "        | That does NOT mean it doesn't define any attenuation."
                << endl;
    } else {
        Argument pos;
        for (std::set<double>::const_iterator fIt = freqEntries.begin(); fIt
                != freqEntries.end(); ++fIt) {
            std::cout << toString((*fIt) / 1e9, 5) << "   | ";
            pos.setArgValue(frequency, *fIt);

            std::map<double, std::set<simtime_t> >::iterator tmpIt =
                    entries.find(*fIt);

            for (std::set<simtime_t>::const_iterator tIt = timeEntries.begin(); tIt
                    != timeEntries.end(); ++tIt) {

                if (tmpIt != entries.end() && tmpIt->second.find(*tIt)
                        != tmpIt->second.end()) {
                    pos.setTime(*tIt);
                    std::cout << toString((FWMath::mW2dBm(m->getValue(pos))), 6) << " ";
                } else {
                    std::cout << "       ";
                }
            }
            std::cout << endl;
        }
    }
    std::cout << "--------+---------------------------------------------------------"
            << endl;
}

//----------Utility methods----------------------------
double EmpiricDecider::toDecibel(double v) {
    return 10.0 * log10(v);
}

std::string EmpiricDecider::toString(simtime_t v, unsigned int length) {
    return toString(SIMTIME_DBL(v), length);
}

template<class T>
std::string EmpiricDecider::toString(T v, unsigned int length) {
    char* tmp = new char[255];
    sprintf(tmp, "%.2f", v);

    std::string result(tmp);
    delete[] tmp;
    while (result.length() < length)
        result += " ";
    return result;
}
