
#include <node_handler.h>
#include "Airframe.h"
#include "logging.h"

Define_Module(cometos::node_handler);

namespace cometos {

void node_handler::finish() {
	recordScalar("sendCounter", sendCounter);
	recordScalar("receiveCounter", receiveCounter);

}

void node_handler::initialize() {


    // dafuq DA steckt keine funktionalität drinne
	//Endpoint::initialize();

	sendCounter = 0;
	receiveCounter = 0;
	dst = BROADCAST; // broadcast ist 2^16 was vorzeichen behaftet minus eins ist
	interval = 2000;
	payloadSize = 50;

	CONFIG_NED(dst);
	CONFIG_NED(interval);
	CONFIG_NED(payloadSize);

	remoteDeclare(&node_handler::get, "get");

	//if (interval > 0) {
	  if (par("initiator"))    {
#ifdef OMNETPP
		start((static_cast<timeOffset_t>(par("start"))));
#endif
}			}

void node_handler::start(timeOffset_t offset) {
	schedule(new Message, &node_handler::traffic, offset);
}

void node_handler::traffic(Message *timer) {
	// create dummy data for transmission
	AirframePtr frame = make_checked<Airframe>();
	for (uint8_t i = 0; i < payloadSize; i++) {
		(*frame) << (uint8_t) i;
	}

	LOG_INFO("send "<<frame->getLength()<<" bytes to "<<dst);
	sendRequest(new DataRequest(dst, frame, createCallback(&node_handler::resp)));

	sendCounter++;

	schedule(timer, &node_handler::traffic, interval);
}

void node_handler::resp(DataResponse *response) {
	LOG_INFO("rcv rsp "<<(int)response->status);
	delete response;
}

void node_handler::handleIndication(DataIndication* msg) {
	LOG_INFO("recv "<<(int)msg->getAirframe().getLength()<<" bytes from "<<msg->src);
	delete msg;
	receiveCounter++;
}
StaticSList<uint8_t, TRAFFIC_MAX_PAYLOAD> node_handler::get(uint8_t& length,
		uint8_t& start) {
    StaticSList<uint8_t, TRAFFIC_MAX_PAYLOAD> ret;

	for (uint8_t i = 0; i < length; i++) {
		ret.push_back(start++);
	}

	return ret;

}

// was hatte das in traffic ursprünglich für ien sinn?
//void serialize(ByteVector & buf,
//		const StaticSList<uint8_t, TRAFFIC_MAX_PAYLOAD> & list) {
//	for (uint8_t it = list.begin(); it != list.end(); it = list.next(it)) {
//		serialize(buf, list.get(it));
//	}
//	serialize(buf, list.size());
//
//	//cout << "Serialized List, size " << (int) buf.getSize() << endl;
//}
//
//void unserialize(ByteVector & buf, StaticSList<uint8_t, TRAFFIC_MAX_PAYLOAD> & list) {
//	//cout << "Unserialize List, size " << (int) buf.getSize() << endl;
//
//	uint8_t size;
//	list.clear();
//	unserialize(buf, size);
//
//	uint8_t item;
//	for (uint8_t i = 0; i < size; i++) {
//		unserialize(buf, item);
//		list.push_front(item);
//	}
//}
//uint8_t getTrafficPayload(StaticSList<uint8_t, TRAFFIC_MAX_PAYLOAD> & list
//		, uint8_t index) {
//	uint8_t it = list.begin();
//	for (; it != list.end() && index != 0; it = list.next(it)) {
//		index--;
//	}
//	if (it != list.end())
//		return list[it];;
//	return 0;
//}

} /* namespace cometos */
