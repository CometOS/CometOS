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
 * @author Stefan Unterschuetz
 * @author Andreas Weigel
 * @author Martin Ringwelski
 */

#include "logging.h"
#include "MacAbstractionLayer.h"
#include "DeciderResultEmpiric802154.h"
#include "DeciderResult802154Narrow.h"
#include <MacToPhyControlInfo.h>
#include <BaseDecider.h>
#include <PhyToMacControlInfo.h>
#include "InvalidLqiProvider.h"

#include "XMLParseUtil.h"

#include "MacHeader.h"
#include "MacPacket.h"
#include "MacControl.h"
#include "palLocalTime.h"
#include "palId.h"

// TODO refactoring opportunity: could be much easier to understand by applying the state pattern
// TODO if we want to be compliant to the basic 802.15.4 transmission procedure, the SIFS (and LIFS)
//      usage in this layer need to be revisited, i.e. instead of SIFS, use LIFS after TX of frames
//      > 18 bytes and do not schedule SIFS after RX of a packet at all!

namespace cometos {

using namespace omnetpp;

Define_Module(MacAbstractionLayer);


/* PUBLIC INTERFACE----------------------------------------------------------*/
MacAbstractionLayer::MacAbstractionLayer(const char * name,
        const node_t* const address) :
        MacAbstractionBase(name),
        FSM<MacAbstractionLayer, MacEvent>(&MacAbstractionLayer::stateIdle),
        sendingSucceed(0),
        sendingFailed(0),
        txDuringRx(0),
        rxDuringTx(0),
        received(0),
		macResetCcaBe(false),
		nwkId(0x0),
        remoteRssi(RSSI_INVALID),
        ackRssi(RSSI_INVALID),
		txSeq(0),
        suppressRxWhileTxPending(false),
        enable(true),
		lqiProvider(0),
        failure(false),
        nb(0),
        accBackoffs(0),
        retryTx(0),
        sendTime(0),
		promiscuousMode(false),
		gateIndOut(this, "gateIndOut"),
        gateSnoopIndOut(this, "gateSnoopIndOut")
{
    ASSERT(address == NULL);
}

MacAbstractionLayer::~MacAbstractionLayer() {
    delete timer;
    timer = nullptr;

    delete lqiProvider;
    lqiProvider = nullptr;

    this->txPkt.force_reset();
}

void MacAbstractionLayer::finish() {
	cancel(timer);
	recordScalar("sendingSucceed", sendingSucceed);
	recordScalar("sendingFailed", sendingFailed);
	recordScalar("txDuringRx", txDuringRx);

	recordScalar("rxDroppedWithInterference", dataFrames.droppedWithInterference);
	recordScalar("rxDroppedWithoutInterference", dataFrames.droppedWithoutInterference);
	recordScalar("rxFailTxAlready", dataFrames.failTxAlready);
	recordScalar("rxFailTxDuringRx", dataFrames.failTxDuringRx);
	recordScalar("rxSuccessWithInterference", dataFrames.successWithInterference);
	recordScalar("rxSuccessWithoutInterference", dataFrames.successWithoutInterference);

	recordScalar("ackDroppedWithInterference", ackFrames.droppedWithInterference);
    recordScalar("ackDroppedWithoutInterference", ackFrames.droppedWithoutInterference);
    recordScalar("ackFailTxAlready", ackFrames.failTxAlready);
    recordScalar("ackFailTxDuringRx", ackFrames.failTxDuringRx);
    recordScalar("ackSuccessWithInterference", ackFrames.successWithInterference);
    recordScalar("ackSuccessWithoutInterference", ackFrames.successWithoutInterference);

    delete timer;
    timer = nullptr;

    delete lqiProvider;
    lqiProvider = nullptr;
}

void MacAbstractionLayer::setLqiProvider(LqiProvider * provider) {
    ASSERT(provider != 0);
    delete lqiProvider;
    lqiProvider = provider;
}

bool MacAbstractionLayer::listen() {
	if (txPkt) {
		//|| phy->getRadioState() == Radio::TX) {
		LOG_ERROR("sending...listen failed");// for debugging
		return false;
	}

	LOG_DEBUG("SetRadioState: " << Radio::RX);
	phy->setRadioState(Radio::RX);
	return true;

}

bool MacAbstractionLayer::sleep() {
	if (txPkt || phy->getRadioState() == Radio::TX
			|| phy->getRadioState() == Radio::SWITCHING) {
		LOG_ERROR("Sleep failed");
		//	ASSERT(false);
		return false;
	}

	LOG_DEBUG("SetRadioState: " << Radio::SLEEP);
	phy->setRadioState(Radio::SLEEP);

	return true;
}

void MacAbstractionLayer::setPromiscuousMode(bool value) {
	promiscuousMode = value;
}


void MacAbstractionLayer::initialize() {
	Module::initialize();

	lowerGateIn = gateBaseId("lowerGateIn");
	lowerGateOut = gateBaseId("lowerGateOut");
	lowerControlIn = gateBaseId("lowerControlIn");
	lowerControlOut = gateBaseId("lowerControlOut");

	CONFIG_NED( bitrate);
	CONFIG_NED( txPower);
	CONFIG_NED( minTxPower);
	CONFIG_NED( maxTxPower);
	CONFIG_NED( ccaDetectionTime);
	CONFIG_NED( macMaxCSMABackoffs);
	CONFIG_NED( aUnitBackoffPeriod);
	CONFIG_NED( macCcaMode);
	CONFIG_NED( macMinBE);
	CONFIG_NED( macMaxBE);
	CONFIG_NED( macMaxFrameRetries);
	CONFIG_NED( sifs);
	CONFIG_NED( macAckWaitDuration);
	//	CONFIG_NED( ackLength);
	CONFIG_NED( aTurnaroundTime);
	CONFIG_NED( txWaitsForBusyRx);
	CONFIG_NED( ccaThreshold);

	CONFIG_NED( macResetCcaBe);
	CONFIG_NED( suppressRxWhileTxPending);

	// avoid calculating dBm2mW every time
	ccaThreshold = FWMath::dBm2mW(ccaThreshold);

	ASSERT(!txPkt);
	timer = new Message;

	// initialize with default LqiProvider
	lqiProvider = new InvalidLqiProvider();

	phy = FindModule<MacToPhyInterface*>::findSubModule(getParentModule());
	ASSERT(NULL != phy);

	cXMLElement* configFile = NULL;
	CONFIG_NED(configFile);

	if (configFile != NULL) {
        cXMLElementList nodes = configFile->getElementsByTagName("node");

        for (cXMLElementList::iterator itNodes = nodes.begin(); itNodes
                != nodes.end(); itNodes++) {
            node_t nodeID = XMLParseUtil::getAddressFromAttribute((*itNodes), "id");
            if (nodeID == getId()) {
                cXMLElementList failureList = (*itNodes)->getElementsByTagName("failure");
                cXMLElementList resumeList = (*itNodes)->getElementsByTagName("resume");
                LOG_DEBUG("Setting failure times");
                createFailureResumeEvents(failureList,
                        createCallback(&MacAbstractionLayer::MACFailed));
                LOG_DEBUG("Setting resume times");
                createFailureResumeEvents(resumeList,
                        createCallback(&MacAbstractionLayer::MACWorks));
            }
        }
	} else {
	    LOG_DEBUG("No Node Failures");
	}
	run();
}

void MacAbstractionLayer::createFailureResumeEvents(cXMLElementList& list,
            const TypedDelegate<LongScheduleMsg> & handler) {
    for (cXMLElementList::iterator it = list.begin();
            it != list.end();
            it++)
    {
        const char * tmp = (*it)->getAttribute("simTime");
        ASSERT(tmp != NULL);
        longTimeOffset_t sTime = atoi(tmp);
        LOG_DEBUG("Event at " << sTime);
        LongScheduleMsg* lsm = new LongScheduleMsg(handler);
        longSchedule(lsm, sTime);
    }
}

/* STATE MACHINE ------------------------------------------------------------*/

fsmReturnStatus MacAbstractionLayer::sendRequest() {
    sendTime = palLocalTime_get();
    if (phy->getRadioState() != Radio::RX) {
        return FSM_HANDLED;
    }

    ASSERT(txPkt);

    scheduleBackoff();
    return transition(&MacAbstractionLayer::stateBackoff);
}

fsmReturnStatus MacAbstractionLayer::stateIdle(MacEvent& e) {
	LOG_DEBUG("idle " << (int) e.signal);

	if (e.signal == MacEvent::SEND_REQUEST) {
	    return sendRequest();
	} else if (MacEvent::ACKED_FRAME_RECEIVED == e.signal) {
	    return transition(&MacAbstractionLayer::stateTransmitAck);
	} else if (MacEvent::FRAME_RECEIVED == e.signal || MacEvent::EXPECTED_FRAME_DROPPED == e.signal) {
		// TODO do we really have to wait a whole SIFS period after RX?
		scheduleTimer(sifs, MacEvent::TIMEOUT_SIFS, "SIFS");
        return transition(&MacAbstractionLayer::stateWaitSIFS);
	} else if (MacEvent::RADIO_SWITCHING_OVER == e.signal) {
		if (phy->getRadioState() == Radio::RX && txPkt) {
	        return sendRequest();
		}
        return FSM_IGNORED;
    } else if(e.signal == FSMEvent::ENTRY_SIGNAL || e.signal == FSMEvent::EXIT_SIGNAL) {
        return FSM_IGNORED;
	} else {
		LOG_ERROR("Error; unknown EVENT: " << (int) e.signal);
		ASSERT(false);
        return FSM_IGNORED;
	}

}

fsmReturnStatus MacAbstractionLayer::stateBackoff(MacEvent& e) {
	LOG_DEBUG("backoff " << (int) e.signal);

	if (e.signal == MacEvent::TIMEOUT_BACKOFF) {

		if (txMode & TX_MODE_CCA) {
			scheduleTimer(ccaDetectionTime, MacEvent::TIMEOUT_CCA, "CCA");
		} else {
			scheduleTimer(0, MacEvent::TIMEOUT_CCA, "NO CCA");
		}
		return transition(&MacAbstractionLayer::stateCCA);
	}
	// receiving frame during backoff
	else if (MacEvent::ACKED_FRAME_RECEIVED == e.signal) {
		return transition(&MacAbstractionLayer::stateTransmitAck);
	} else if (MacEvent::FRAME_RECEIVED == e.signal || MacEvent::EXPECTED_FRAME_DROPPED == e.signal) {
		cancel(timer); // cancel backoff timer
		// TODO do we really have to wait a whole SIFS period after RX?
		scheduleTimer(sifs, MacEvent::TIMEOUT_SIFS, "SIFS");
        return transition(&MacAbstractionLayer::stateWaitSIFS);
	} else if (MacEvent::RADIO_SWITCHING_OVER == e.signal) {
	    return FSM_IGNORED;
	} else if(e.signal == FSMEvent::ENTRY_SIGNAL || e.signal == FSMEvent::EXIT_SIGNAL) {
        return FSM_IGNORED;
	} else {
		LOG_ERROR("Error; unknown EVENT: " << (int) e.signal);
		ASSERT(false);
		return FSM_IGNORED;
	}
}

bool MacAbstractionLayer::isChannelBusy() {
    ChannelState cs = phy->getChannelState();
    bool channelBusy = false;
    switch (macCcaMode){
    case MAC_CCA_MODE_ENERGY_OR_CS:
        channelBusy = !cs.isIdle() || (cs.getRSSI() > ccaThreshold);
        break;
    case MAC_CCA_MODE_ENERGY:
        channelBusy = cs.getRSSI() > ccaThreshold;
        break;
    case MAC_CCA_MODE_CS:
        channelBusy = !cs.isIdle();
        break;
    case MAC_CCA_MODE_ENERGY_AND_CS:
        channelBusy = !cs.isIdle() && (cs.getRSSI() > ccaThreshold);
        break;
    default:
        ASSERT(false);
        break;
    }
    return channelBusy;
}

fsmReturnStatus MacAbstractionLayer::stateCCA(MacEvent& e) {
	LOG_DEBUG("cca " << (int) e.signal);

	if (e.signal == MacEvent::TIMEOUT_CCA) {

		ASSERT(phy->getRadioState() == Radio::RX);
		ASSERT(txPkt);

		ChannelState cs = phy->getChannelState();


		// TODO MiXiM currently does not seem to allow for switching to
		// a TX-pending mode (on hardware: e.g., PLL_ON state) which prevents
		// reception but at the same time allows sensing the channel RSSI. We
		// could switch to TX as soon as we start sending, but then the CCA
		// would always yield an RSSI of zero, because of the
		// RadioStateAnalogue model, which adds an total attenuation to all
		// incoming signals
		// Explicitly requesting a channel sense from the PHY (which is
		// possible) does not change this, because it should also yields an
		// RSSI of 0 when used with Radio::TX state.

		// this would be needed to more accurately resemble the behaviour
		// of hardware-assisted transmission, e.g. of Atmel's RF230/RFA1,
		// which enter TX_ARET_ON state and thereby prevent any reception
		// during backoffs -- on the other hand, this isn't a good idea
		// anyway

		// check for a busy channel or if we are already sending (e.g., an ACK)
		if ((isChannelBusy() && ((txMode & TX_MODE_CCA) != 0))
				|| phy->getRadioState() == Radio::TX
				|| phy->getRadioState() == Radio::SWITCHING) {
				
			// channel is busy
		    state_t nextState;
			if (nb < macMaxCSMABackoffs) {
				nb++;
				accBackoffs++;

				nextState = &MacAbstractionLayer::stateBackoff;
				scheduleBackoff();
			} else {
				nextState = &MacAbstractionLayer::stateIdle;
				scheduleTxDone(MTR_CHANNEL_ACCESS_FAIL);
			}

			LOG_DEBUG("cca fail with idle=" << cs.isIdle() << " & RSSI="
			                    << FWMath::mW2dBm(cs.getRSSI()) << " & phyRadioState=" << phy->getRadioState());

			return transition(nextState);
		} else {
		
			// channel is considered free
			LOG_DEBUG("cca success with idle=" << cs.isIdle() << " & RSSI="
					<< FWMath::mW2dBm(cs.getRSSI()));
			// reset backoffing
			nb = 0;

			txPkt->removeAll(); // remove all meta data

			MacPacket *macPkt = new MacPacket();
			macPkt->encapsulateArray(txPkt->getData(), txPkt->getLength());

			// possibly need meta data for empirical physical layer
			// TODO quite dirty hack --- better solution?
			macPkt->meta.set(new NodeId(getId()));


			simtime_t switchTime = phy->setRadioState(Radio::TX);
			LOG_DEBUG("setRadioState(" << Radio::TX << "); retValue=" << switchTime);
			ASSERT(switchTime.dbl() >= 0);

//			attachSignal(macPkt, simTime() + aTurnaroundTime, currTxPower);
//			sendDelayed(macPkt, aTurnaroundTime, lowerGateOut);

			// while the original MiXiM csma.cc uses this rather strange 
			// delay by aTurnaroundTime before sending, this does not reflect 
			// reality and DOES have a quite measurable influence on interference
			// drops, drops due to a TX state of a node etc. --- we therefore
			// now use the more realistic RX-TX switch time (during which 
			// the RadioStateAnalougeModel should set the attenuation factor to
			// 0.0 (meaning the incoming signal has a power of 0 Watt)
            attachSignal(macPkt, simTime() + switchTime, currTxPower);
            sendDelayed(macPkt, switchTime, lowerGateOut);

			return transition(&MacAbstractionLayer::stateTransmitFrame);
			
		}
	}
	// receiving frame during cca
	else if (MacEvent::ACKED_FRAME_RECEIVED == e.signal) {
		return transition(&MacAbstractionLayer::stateTransmitAck);
	} else if (MacEvent::FRAME_RECEIVED == e.signal || MacEvent::EXPECTED_FRAME_DROPPED == e.signal) {
	    // TODO do we really have to wait a whole SIFS period after RX?
		cancel(timer); // cancel cca timer
		scheduleTimer(sifs, MacEvent::TIMEOUT_SIFS, "SIFS");
        return transition(&MacAbstractionLayer::stateWaitSIFS);
	} else if (MacEvent::RADIO_SWITCHING_OVER == e.signal) {
        return FSM_IGNORED;
	} else if (MacEvent::ACK_RECEIVED == e.signal) {
		ASSERT(false);
        return FSM_IGNORED;
    } else if(e.signal == FSMEvent::ENTRY_SIGNAL || e.signal == FSMEvent::EXIT_SIGNAL) {
        return FSM_IGNORED;
	} else {
		LOG_ERROR("Error; unknown EVENT: " << (int) e.signal);
		ASSERT(false);
        return FSM_IGNORED;
	}

}

fsmReturnStatus MacAbstractionLayer::stateTransmitFrame(MacEvent& e) {

	LOG_DEBUG("trnsmt " << (int) e.signal);

	if (e.signal == MacEvent::FRAME_TRANSMITTED) {
		ASSERT(txPkt);
		if (txMode & TX_MODE_AUTO_ACK) {
			scheduleTimer(macAckWaitDuration, MacEvent::TIMEOUT_ACK, "ACK_W");
            return transition(&MacAbstractionLayer::stateWaitAck);
		} else {
			scheduleTxDone(MTR_SUCCESS);
            return transition(&MacAbstractionLayer::stateIdle);
		}
	} else if (MacEvent::RADIO_SWITCHING_OVER == e.signal || MacEvent::EXPECTED_FRAME_DROPPED == e.signal) {
	    // ignore dropped packets here, they are a result from a simulated
	    // RX which started during our backoff or CCA
	    // (before we switch Radio to TX and thereby suppress any incoming signals)
        return FSM_IGNORED;
    } else if(e.signal == FSMEvent::ENTRY_SIGNAL || e.signal == FSMEvent::EXIT_SIGNAL) {
        return FSM_IGNORED;
	} else {
		LOG_ERROR("Error; unknown EVENT: " << (int) e.signal);
		ASSERT(false);
        return FSM_IGNORED;
	}

}

fsmReturnStatus MacAbstractionLayer::stateWaitAck(MacEvent& e) {
	LOG_DEBUG("ack wait " << (int) e.signal);

	if (e.signal == MacEvent::TIMEOUT_ACK) {
		if (retryTx < currMaxFrameRetries) {
			LOG_INFO("no ack received, retry " << retryTx);
			nb = 0;

			if (!macResetCcaBe) {
			    currMinBe = currMinBe + 1 <= macMaxBE ? currMinBe + 1 : macMaxBE;
			}
			retryTx++;
			scheduleBackoff();
			return transition(&MacAbstractionLayer::stateBackoff);
		} else {
			LOG_INFO("no ack received, discard " << retryTx);
			scheduleTxDone(MTR_NO_ACK);
            return transition(&MacAbstractionLayer::stateIdle);
		}
	} else if (e.signal == MacEvent::ACK_RECEIVED) {
		cancel(timer);
		scheduleTxDone(MTR_SUCCESS);
        return transition(&MacAbstractionLayer::stateIdle);
	} else if (MacEvent::RADIO_SWITCHING_OVER == e.signal || e.signal == MacEvent::EXPECTED_FRAME_DROPPED) {
        return FSM_IGNORED;
    } else if(e.signal == FSMEvent::ENTRY_SIGNAL || e.signal == FSMEvent::EXIT_SIGNAL) {
        return FSM_IGNORED;
	} else {
		LOG_ERROR("Error; unknown EVENT: " << (int) e.signal);
		ASSERT(false);
        return FSM_IGNORED;
	}

}

fsmReturnStatus MacAbstractionLayer::stateWaitSIFS(MacEvent& e) {

	LOG_DEBUG("sifs " << (int) e.signal);

	if (e.signal == MacEvent::TIMEOUT_SIFS) {
		if (txPkt) {
		    return sendRequest();
		} else {
		    return transition(&MacAbstractionLayer::stateIdle);
		}
        return FSM_IGNORED;
	} else if (MacEvent::SEND_REQUEST == e.signal || MacEvent::EXPECTED_FRAME_DROPPED == e.signal) {
		// sending request is processed after sifs period
        return FSM_IGNORED;
	} else if (MacEvent::RADIO_SWITCHING_OVER == e.signal) {
        return FSM_IGNORED;
    } else if(e.signal == FSMEvent::ENTRY_SIGNAL || e.signal == FSMEvent::EXIT_SIGNAL) {
        return FSM_IGNORED;
	} else {
		LOG_ERROR("Error; unknown EVENT: " << (int) e.signal);
		ASSERT(false);
        return FSM_IGNORED;
	}
}

fsmReturnStatus MacAbstractionLayer::stateTransmitAck(MacEvent& e) {
	LOG_DEBUG("trnsmt ack " << (int) e.signal);

	if (e.signal == MacEvent::FRAME_TRANSMITTED) {
		scheduleTimer(sifs, MacEvent::TIMEOUT_SIFS, "SIFS");
        return transition(&MacAbstractionLayer::stateWaitSIFS);
	} else if (MacEvent::RADIO_SWITCHING_OVER == e.signal || MacEvent::SEND_REQUEST == e.signal || MacEvent::EXPECTED_FRAME_DROPPED == e.signal) {
		// ignore states, handled afterwards
        return FSM_IGNORED;
    } else if(e.signal == FSMEvent::ENTRY_SIGNAL || e.signal == FSMEvent::EXIT_SIGNAL) {
        return FSM_IGNORED;
	} else {
		LOG_ERROR("Error; unknown EVENT: " << (int) e.signal);
		ASSERT(false);
		return FSM_IGNORED;
	}
}

void MacAbstractionLayer::handleMessage(cMessage* msg) {

    if (msg->getArrivalGateId() == lowerControlIn) {
        if (!failure) {
            receiveLowerControl(msg);
        } else {
            LOG_DEBUG("MAC in fail Mode");
            delete msg;
        }
    } else if (msg->getArrivalGateId() == lowerGateIn) {
        if (!failure) {
            receiveLowerData(msg);
        } else {
            LOG_DEBUG("MAC in fail Mode");
            delete msg;
        }
    } else {
        Module::handleMessage(msg);
    }
}

void MacAbstractionLayer::handleRxDrop(const char * frameType,
                  node_t dst,
                  node_t src,
                  const DeciderResultEmpiric802154* dre,
                  FrameStats & stats) {
//    LOG_INFO("DROPPED " << frameType << " frame for " << hex << dst << " from " << src);
    if (dre != NULL) {
        if (dre->getFailTxAlready()) {
            stats.failTxAlready++;
        } else if(dre->getFailTxDuringRx()) {
            stats.failTxDuringRx++;
        } else {
            if (dre->hadInterference()) {
                stats.droppedWithInterference++;
            } else {
                stats.droppedWithoutInterference++;
            }
        }
    }
    rxDropped();
}

void MacAbstractionLayer::logRxFrame(const char * frameType,
                                     node_t dst,
                                     node_t src,
                                     const DeciderResultEmpiric802154* dre,
                                     FrameStats & stats) {

    LOG_INFO("RECEIVED " << frameType << " frame from " << hex << src << " to " << dst);
//    {
//        stringstream ss; ss.precision(9);
//        ss<<simTime().dbl()<<"|"<<getFullName()<<"|0x"<<std::hex<<3<<std::dec<<"|"<<__func__<<"|"<<"RECEIVED " << frameType << " frame from ";
//        ss << hex;
//        ::operator<<(ss, hex);
//        ss << src << " to " << dst<<std::endl;
//        getLogger().log(getName(), 3 , 0, ss.str());
//    }
    if (dre != NULL) {
        if (dre->hadInterference()) {
            stats.successWithInterference++;
        } else {
            stats.successWithoutInterference++;
        }
    }
}

void MacAbstractionLayer::receiveLowerControl(cMessage *msg) {

	LOG_DEBUG("receive ctrl "<<msg->getKind() );

	if (msg->getKind() == MacToPhyInterface::TX_OVER) {
	    LOG_DEBUG("SetRadioState: " << Radio::RX);
		phy->setRadioState(Radio::RX);
		MacEvent e(MacEvent::FRAME_TRANSMITTED);
		dispatch(e);
	} else if (msg->getKind() == BaseDecider::PACKET_DROPPED) {
		//ASSERT(phy->getRadioState() == Radio::RX);
        MacPacket *pktMac = check_and_cast<MacPacket*>(msg);
        AirframePtr pkt = pktMac->decapsulateNewAirframe();

        MacHeader header;
        (*pkt) >> header;

        DeciderResult* dr = check_and_cast<PhyToMacControlInfo*>(pktMac->getControlInfo())->getDeciderResult();

        // use dynamic casts to check if the decider result provides some information
        // about interference status during reception
        DeciderResultEmpiric802154 * dre = dynamic_cast<DeciderResultEmpiric802154*>(dr);

        LOG_INFO("DROPPED pckt for " << header.dst << " from " << header.src);
        if (header.type == MAC_TYPE_DATA) {
            if (header.dst == getId() || header.dst == MAC_BROADCAST) {
                handleRxDrop("data", header.dst, header.src, dre, dataFrames);
            }
        } else if (header.type == MAC_TYPE_ACK) {
            if (header.src == getId()) {
                handleRxDrop("ack", header.src, header.dst, dre, ackFrames);
            }
        }
        pkt.deleteObject();

		// we additionally dispatch a frame dropped event to handle a situation
		// in which we waited with TX for RX of a frame which then failed --
		// we only dispatch the event if the frame was dropped due to a bit
		// error, i.e. simulated CRC failure and NOT due to a missed SFD
		// --- this is also the logic we apply when we decide to dispatch
		// a send request in the first place, i.e. we wait if currently a
		// frame is received
		if (phy->getChannelState().isIdle()) {
		    MacEvent e(MacEvent::EXPECTED_FRAME_DROPPED);
            dispatch(e);
		}
	} else if (msg->getKind() == MacToPhyInterface::RADIO_SWITCHING_OVER) {
	    MacEvent e(MacEvent::RADIO_SWITCHING_OVER);
		dispatch(e);
	} else {
		LOG_ERROR("MsgKind " << msg->getKind());
		ASSERT(false);
	}
	delete msg;
}

void MacAbstractionLayer::rxDropped() {
	LOG_INFO("Pckt dropped");
	// should be implemented by derived class
}

void MacAbstractionLayer::handleAckFromLower(mac_dbm_t rssi,
                                   const MacHeader& header,
                                   const DeciderResultEmpiric802154 * dre) {
#ifdef RSSI_IN_DATA_LINK_ACK
    (*pkt) >> remoteRssi;
#endif
    ackRssi = rssi;
    LOG_DEBUG("RECEIVED ack for pckt from " << STREAM_HEX(header.src) << " to "
                    << STREAM_HEX(header.dst));
    // check whether ack is received
    if (txPkt && header.dstNwkId == nwkId && header.src == getId()
            && header.seq == txSeq) {
        LOG_DEBUG("ACK is for me");
        logRxFrame("ack", header.src, header.dst, dre, ackFrames);
        MacEvent e(MacEvent::ACK_RECEIVED);
        dispatch(e);
    }
}

void MacAbstractionLayer::handleDataFromLower(
              const MacHeader & header,
              AirframePtr pkt,
              const DeciderResultEmpiric802154 * dre,
              lqi_t lqi,
              bool lqiValid,
              mac_dbm_t rssi)
{
    // receception of data during waiting for ack or during
    // an active transmission (e.g. in backoff state) with
    // suppressRxWhileTxPending=true is forbidden
    if (getState() == &MacAbstractionLayer::stateWaitAck
            || (txPkt && suppressRxWhileTxPending)) {
        LOG_DEBUG("DISCARD data from " << cometos::hex
                << header.src << " to "
                << header.dst << " nwkId="
                << (int) header.dstNwkId
                << " supprRxWhileTx=" <<suppressRxWhileTxPending)
        pkt.deleteObject();
        if (phy->getChannelState().isIdle()) {
            MacEvent e(MacEvent::EXPECTED_FRAME_DROPPED);
            dispatch(e);
        }
        return;
    }

    // check if addresses are matching
    bool filterByNetwork = (header.dstNwkId != nwkId && header.dstNwkId != MAC_NWK_BROADCAST);
    bool filterByAddr = (header.dst != getId() && header.dst != MAC_BROADCAST);

    if (!promiscuousMode && (filterByAddr || filterByNetwork)) {
        LOG_INFO("DISCARD data from " << cometos::hex << header.src << " to " << header.dst << " nwkId=" << (int) header.dstNwkId);
        pkt.deleteObject();

        // signal that a frame was received but dropped due to filtering
        if (phy->getChannelState().isIdle()) {
            MacEvent e(MacEvent::EXPECTED_FRAME_DROPPED);
            dispatch(e);
        }
        return;
    }

    logRxFrame("data", header.dst, header.src, dre, dataFrames);

    // TODO for now, we only allow broadcasts to other networks other than ourselves
    //      otherwise we most likely would need some source network for direct ACKs
    if (header.dstNwkId != nwkId) {
        ASSERT(header.dstNwkId == MAC_NWK_BROADCAST);
    }
    if (header.dstNwkId != nwkId) {
        ASSERT(header.dst == MAC_BROADCAST);
    }

    // reply with ack, if packet was unicast
    // actually sending of an ack can be forced by the packet,
    // in AVR this is not possible, thus we check the value for autoack
    if (header.seq && header.dst == getId() && enable == true) {

        // we removed this to allow backoff to continue -- instead, we
        // check here and before actually sending a packet, if we are
        // already in RadioState::TX
//          cancel(timer); // cancel current sending trial
        if (phy->getRadioState() == Radio::TX || phy->getRadioState() == Radio::SWITCHING) {
            // in this case, end of CCA and end of frame are perfectly aligned
            // and we cannot send our ack and have to drop the packet
            LOG_INFO("Sending of ACK aborted due to busy TX");
        } else {
            cancel(timer);
            AirframePtr ack = make_checked<Airframe>();
#ifdef RSSI_IN_DATA_LINK_ACK
            (*ack) << rssi;
#endif
            MacHeader headerAck(header.seq, header.src, header.dst,
                    MAC_TYPE_ACK, nwkId);
            (*ack) << headerAck;

            simtime_t switchTime = phy->setRadioState(Radio::TX);
            LOG_DEBUG("setRadioState(" << Radio::TX << "); retValue=" << switchTime);
            if (switchTime <= 0) {
                ASSERT(switchTime > 0);
            }

            ASSERT(switchTime.dbl() >= 0);
            MacPacket* macPkt = new MacPacket();
            macPkt->encapsulateArray(ack->getData(), ack->getLength());

            // possibly need meta data for empirical physical layer
            // TODO quite dirty hack --- better solution?
            macPkt->meta.set(new NodeId(getId()));
            ack.deleteObject();

            attachSignal(macPkt, simTime() + aTurnaroundTime, txPower);

            LOG_INFO("send ack of size" << " omnetppLen=" << macPkt->getByteLength());
            sendDelayed(macPkt, aTurnaroundTime, lowerGateOut);
            MacEvent e(MacEvent::ACKED_FRAME_RECEIVED);
            dispatch(e);
        }
    } else {
        MacEvent e(MacEvent::FRAME_RECEIVED);
        dispatch(e);
    }

    //scheduleTimer(sifs, MacEvent::TIMEOUT_SIFS, "sifs");
    received++; // logging

    MacRxInfo phyInfo(lqi, lqiValid, rssi, true, palLocalTime_get());

    // call subclass' rxEnd
    rxEnd(pkt, header.src, header.dst, phyInfo);

    // send up indication
    DataIndication * ind = new DataIndication(pkt, header.src, header.dst);
    ind->set((MacRxInfo*) phyInfo.getCopy());
    if (ind->dst == palId_id() || ind->dst == MAC_BROADCAST) {
        gateIndOut.send(ind);
    } else {
        gateSnoopIndOut.send(ind);
    }
}

void MacAbstractionLayer::receiveLowerData(cMessage *msg) {

	// Get data from Mac Packet
	MacPacket *pktMac = check_and_cast<MacPacket*>(msg);
	AirframePtr pkt = pktMac->decapsulateNewAirframe();
	PhyToMacControlInfo* cinfo =
			check_and_cast<PhyToMacControlInfo*>(pktMac->getControlInfo());

	// TODO EmpiricDecider returns a "real" RSSI in units of dbm, while
	// 802154Narrow actually returns the SINR; has to be fixed in MiXiM imho
	const DeciderResultEmpiric802154 * dre = dynamic_cast<const DeciderResultEmpiric802154*>(cinfo->getDeciderResult());
	const DeciderResult802154Narrow * drn = dynamic_cast<const DeciderResult802154Narrow*>(cinfo->getDeciderResult());

	double  rssiDouble = 0.0;
	bool lqiValid = false;
	lqi_t lqi = 0;
	// check the type of result we got to
	if (dre) {
	    rssiDouble = dre->getRSSI();
	    lqi = lqiProvider->calculateLqi(*dre, lqiValid);
	} else if (drn) {
	    rssiDouble = drn->getRSSI();
	    lqi = lqiProvider->calculateLqi(*drn, lqiValid);
	} else {
	    LOG_WARN("Unknown DeciderResult class, no meta data extracted")
	}

	mac_dbm_t rssi;
	if (rssiDouble <= MAC_RSSI_MIN) {
	    rssi = MAC_RSSI_MIN;
	} else {
	    rssi = (int8_t) rssiDouble;
	}
	LOG_DEBUG("rssiOrig=" << rssiDouble << " rssi=" << (int)rssi);

	MacHeader header;
	(*pkt) >> header;

	// data packet
	if (header.type == MAC_TYPE_DATA) {
	    handleDataFromLower(header, pkt, dre, lqi, lqiValid, rssi);
	} else if (header.type == MAC_TYPE_ACK) {
	    handleAckFromLower(rssi, header, dre);
		pkt.deleteObject();
	} else {
		pkt.deleteObject();
		// invalid packet
		ASSERT(false);
	}

	// do not delete earlier, as we pass dre, which is a pointer into cinfo, which
	// is a pointer into pktMac, to the handleData/handleAck methods just above
    delete pktMac;
    pktMac = NULL;
}

void MacAbstractionLayer::rxEnd(AirframePtr pkt, node_t src, node_t dst, MacRxInfo const & info) {
	LOG_ERROR("Discrd data");ASSERT(false);
	pkt.deleteObject();
	// should be implemented by derived class
}

void MacAbstractionLayer::scheduleBackoff() {
	double additionalWait = 0;

	if (txMode & TX_MODE_BACKOFF) {
	    // calculate active backoff exponent depending on adapted
	    // minimum backoff exponent and number of backoffs for this try
	    uint8_t backoffExp = currMinBe + nb;
	    backoffExp = backoffExp <= macMaxBE ? backoffExp : macMaxBE;
	    // intrand(r) draws a number from [0,r), effectively doing the "-1" calculation from the standard here
	    double bo = intrand( ((uint16_t) 1) << backoffExp) * aUnitBackoffPeriod + additionalWait;
	    LOG_DEBUG("Schedule backoff (aUBo=" << aUnitBackoffPeriod << "|backoffExp=" << (int) backoffExp << "|currMinBe=" <<  currMinBe << ") " << bo);
	    scheduleTimer(bo,
              MacEvent::TIMEOUT_BACKOFF, "backoff");
	} else {
		scheduleTimer(additionalWait, MacEvent::TIMEOUT_BACKOFF, "no backoff");
	}
}

void MacAbstractionLayer::scheduleTimer(double time, uint8_t kind,
		const char* name) {

	if (timer->isScheduled() != false) {
		LOG_DEBUG(" current " << (int) timer->getKind() << "   should "
				<< (int) kind);

	}

	ASSERT(timer->isScheduled() == false);

	timer->setKind(kind);
	timer->setName(name);

	schedule_dbl(timer, &MacAbstractionLayer::timeout, time);

}

void MacAbstractionLayer::timeout(Message *msg) {
	MacEvent e = (MacEvent) msg->getKind();
	if((e.signal == MacEvent::TIMEOUT_CCA)
	         || (e.signal == MacEvent::TIMEOUT_BACKOFF)
	         || (e.signal == MacEvent::TIMEOUT_ACK)
	         || (e.signal == MacEvent::TIMEOUT_SIFS)) {
	    dispatch(e);
	} else if (e.signal == MacEvent::TX_DONE) {
	    txDone(lastResult);
	} else {
	    LOG_ERROR("Unknown kind of timer fired: " << (int) e.signal);
	    ASSERT(false);
	}
}

bool MacAbstractionLayer::sendAirframe(AirframePtr frame, node_t dst, uint8_t mode, const ObjectContainer* meta) {
    if (failure == true) {
        LOG_DEBUG("MAC in fail Mode");
        frame.deleteObject();
        return false;
    }

    // discard frames send to our own address
    if (dst == palId_id()) {
        LOG_WARN("discarded frame to this node's address")
        frame.deleteObject();
        return false;
    }

    mac_networkId_t dstNwk = nwkId;;

	if (txPkt) {
		frame.deleteObject();
		LOG_DEBUG("abort: currently sending");ASSERT(false);
		return false;
	}

	// SEND PACKET /////////////////////////////
	currTxPower = txPower;
	currMaxFrameRetries = macMaxFrameRetries;

	if (meta != NULL) {
        MacControl* control = meta->getUnsafe<MacControl>();
        if (control) {
            currTxPower = (maxTxPower - minTxPower) * (control->txPower / 255.0)
                    + minTxPower;
        }

        MacMetaRetries* controlRetries = meta->getUnsafe<MacMetaRetries>();
        if (controlRetries) {
            currMaxFrameRetries = controlRetries->_retries;
            LOG_DEBUG("set retries to: " << currMaxFrameRetries);
        } else {
            LOG_DEBUG("default retries: " << currMaxFrameRetries);
        }

        if (meta->has<DestinationNwk>()) {
            mac_networkId_t tmp = meta->get<DestinationNwk>()->dstNwk;
            LOG_DEBUG("SEND TO NETWORK nwkId=" << (int) tmp);
            dstNwk = tmp;
        }
	}

	return sendToNetwork(frame, dst, dstNwk, mode);
}

bool MacAbstractionLayer::sendToNetwork(AirframePtr frame, node_t dst,
		mac_networkId_t dstNwk, uint8_t mode) {

	LOG_DEBUG("sending pckt");

	if (enable == false) {
		frame.deleteObject();
		return false;
	}

	// store  and prepare packet for transmission
	txPkt = frame;
	txMode = mode;

	while (txSeq == 0) {
		txSeq++;
	}

	if (dst == MAC_BROADCAST) {
		txMode &= ~(TX_MODE_AUTO_ACK);
	}

	// initialize and append information to packet
	MacHeader header(txSeq, getId(), dst, MAC_TYPE_DATA, dstNwk);

	if (txMode & (TX_MODE_AUTO_ACK)) {
		// seqNum already set correctly
	} else {
		// no ACK expected, no seqNum needed
		header.seq = 0;
	}
	(*frame) << header;

	// resetting parameters for transmission
	resetTxParams();

	bool isReceiving = !phy->getChannelState().isIdle();
	bool startSendingImmediately = false;

	if (!txWaitsForBusyRx) {
	    startSendingImmediately = true;
	}


	// check if we are currently receiving anything
	if (isReceiving) {
	    // the phy channel is currently trying to decode an incoming frame,
        // record this
	    txDuringRx++;
	} else {

	    // if the channel is idle, sending start in any case
	    startSendingImmediately = true;
	}

	if (startSendingImmediately) {
        LOG_DEBUG("init sending process");
        MacEvent e(MacEvent::SEND_REQUEST);
        dispatch(e);
	}

	return true;
}

// TODO this is redundant with the implementation in Addressing
//      we did not want to pull in dependencies to a certain addressing scheme
//      into the cometos MAC (not yet, at least), therefore this ugly hack
bool MacAbstractionLayer::setNwkId(mac_networkId_t id) {
	if (id != MAC_NWK_BROADCAST) {
		LOG_INFO("SET nwkId to " << (int) id);
		this->nwkId = id;
		return true;
	} else {
		return false;
	}
}

node_t MacAbstractionLayer::getShortAddr() {
	return getId();
}

void MacAbstractionLayer::scheduleTxDone(macTxResult_t result) {
    lastResult = result;
    scheduleTimer(0, MacEvent::TX_DONE, "txdone");
}

void MacAbstractionLayer::txDone(macTxResult_t result) {
	LOG_DEBUG("Sig txDone for pckt with ID");

	ASSERT(txPkt);

	MacHeader header;
	(*txPkt) >> header;
	MacTxInfo info(header.dst, retryTx);
#ifdef RSSI_IN_DATA_LINK_ACK
	info.remoteRssi = remoteRssi;
#endif
	info.numCCARetries = accBackoffs;
	info.ackRssi = ackRssi;
	info.txDuration = palLocalTime_get() - sendTime;

	// make sure that ackRssi is only set to valid directly after receiving ACK
	ackRssi = RSSI_INVALID;

	txPkt.deleteObject();

	if (result == MTR_SUCCESS)
		sendingSucceed++;
	else
		sendingFailed++;

	txEnd(result, info);
}

void MacAbstractionLayer::txEnd(macTxResult_t result, MacTxInfo const & info) {
	LOG_INFO("Tx end");
	// should be implemented by derived class
	ASSERT(false);
}

void MacAbstractionLayer::attachSignal(cPacket* mac, simtime_t startTime,
		double power) {

	simtime_t duration = (mac->getBitLength() + phy->getPhyHeaderLength())
			/ bitrate;

	Signal* s = createSignal(startTime, duration, power, bitrate);
	MacToPhyControlInfo* cinfo = new MacToPhyControlInfo(s);

	mac->setControlInfo(cinfo);
}

Signal* MacAbstractionLayer::createSignal(simtime_t start, simtime_t length,
		double power, double bitrate) {
	simtime_t end = start + length;
	//create signal with start at current simtime and passed length
	Signal* s = new Signal(start, length);

	//create and set tx power mapping
	Mapping* txPowerMapping = createRectangleMapping(start, end, power);
	s->setTransmissionPower(txPowerMapping);

	//create and set bitrate mapping
	Mapping* bitrateMapping = createConstantMapping(start, end, bitrate);
	s->setBitrate(bitrateMapping);

	return s;
}

Mapping* MacAbstractionLayer::createRectangleMapping(simtime_t start,
		simtime_t end, double value) {
	//create mapping over time
	Mapping* m = MappingUtils::createMapping(DimensionSet::timeDomain,
			Mapping::LINEAR);

	//set position Argument
	Argument startPos(start);
	//set discontinuity at position
	MappingUtils::addDiscontinuity(m, startPos, 0.0, MappingUtils::post(start),
			value);

	//set position Argument
	Argument endPos(end);
	//set discontinuity at position
	MappingUtils::addDiscontinuity(m, endPos, 0.0, MappingUtils::pre(end),
			value);

	return m;
}

Mapping* MacAbstractionLayer::createConstantMapping(simtime_t start,
		simtime_t end, double value) {
	//create mapping over time
	Mapping* m = MappingUtils::createMapping(0.0, DimensionSet::timeDomain,
			Mapping::LINEAR);

	//set position Argument
	Argument startPos(start);

	//set mapping at position
	m->setValue(startPos, value);

	//set position Argument
	Argument endPos(end);

	//set mapping at position
	m->setValue(endPos, value);

	return m;
}

void MacAbstractionLayer::resetTxParams() {
    currMinBe = macMinBE;
    nb = 0;
    accBackoffs = 0;
    retryTx = 0;
}

uint16_t MacAbstractionLayer::getBackoffTime(uint8_t be) {
	return (1000 * pow(2.0, be) - 1) * aUnitBackoffPeriod;
}

bool MacAbstractionLayer::configureBackoffAlgorithm(uint8_t minBE,
		uint8_t maxBE, uint8_t maxFrameRetries) {
	if (macMinBE > macMaxBE) {
		return false;
	}
	macMaxBE = maxBE;
	macMinBE = minBE < macMaxBE ? minBE : macMaxBE;
	macMaxFrameRetries = maxFrameRetries;

	return true;
}

}
