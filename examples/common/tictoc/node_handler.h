
#ifndef PERIODICTRAFFIC_H_
#define PERIODICTRAFFIC_H_

#include "Endpoint.h"

#define TRAFFIC_MAX_PAYLOAD		20

namespace cometos {

class node_handler: public cometos::Endpoint {
public:
	/**Sets parameters*/
	void initialize();

	/**Periodically generates traffic
	 */
	void traffic(Message *timer);

	void finish();

	/**Handles response message*/
	void resp(DataResponse *response);

	void handleIndication(DataIndication* msg);

	/**Debuugin Method for RMI*/
	StaticSList<uint8_t, TRAFFIC_MAX_PAYLOAD> get(uint8_t& length, uint8_t& start);

	void start(timeOffset_t offset);

	int sendCounter;
	int receiveCounter;

	node_t dst;
	timeOffset_t interval;
	pktSize_t payloadSize;
};
/// additional serialization routines


uint8_t getTrafficPayload(StaticSList<uint8_t, TRAFFIC_MAX_PAYLOAD> & list, uint8_t index);


// eventuell auch weg, siehe .cc
void serialize(ByteVector & buf, const StaticSList<uint8_t, TRAFFIC_MAX_PAYLOAD> & list);
void unserialize(ByteVector & buf, StaticSList<uint8_t, TRAFFIC_MAX_PAYLOAD> & list);




} /* namespace cometos */
#endif /* PERIODICTRAFFIC_H_ */
