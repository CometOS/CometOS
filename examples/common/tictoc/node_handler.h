
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
	void flood_network(Message *timer);
	void generate_events(Message *timer);

	void finish();

	/**Handles response message*/
	void resp(DataResponse *response);

	void handleIndication(DataIndication* msg);

	void send_event();

	uint32_t read_uint32_t(DataIndication* msg,int offset_in_byte = 0);

	/**Debuugin Method for RMI*/
	StaticSList<uint8_t, TRAFFIC_MAX_PAYLOAD> get(uint8_t& length, uint8_t& start);

	void start(timeOffset_t offset);




	static bool running;
	static unsigned int messagesSend;



	                                   // Importent for |Coordinator|Client|
	unsigned long int counter;                // uint32 |   X       |      | counts events
	unsigned long int threshold;              // uint32 |   X       |      | truly a threshold
	unsigned long int distanceToCoordinator;  // uint32 |           |  X   | says how many messages it takes to inform the coordinator for a event
	timeOffset_t slf_msg_timer;               // uint16 |           |      | time to next action (right now not used)
	node_t out;                               // uint16 |           |  X   | gives a node the output for events (there is always just one valid output in the direction of coordinator)
	bool isSet;                               //        |           |  X   | true if out isSet           (not importent for the Coordinator)
	//pktSize_t payloadSize;                  // uint8  |           |      | also apparently not in usage right now


private:
	int sendCounter;

};


/// additional serialization routines


uint8_t getTrafficPayload(StaticSList<uint8_t, TRAFFIC_MAX_PAYLOAD> & list, uint8_t index);


// eventuell auch weg, siehe .cc
void serialize(ByteVector & buf, const StaticSList<uint8_t, TRAFFIC_MAX_PAYLOAD> & list);
void unserialize(ByteVector & buf, StaticSList<uint8_t, TRAFFIC_MAX_PAYLOAD> & list);




} /* namespace cometos */
#endif /* PERIODICTRAFFIC_H_ */
