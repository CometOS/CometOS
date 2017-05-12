
#ifndef PERIODICTRAFFIC_H_
#define PERIODICTRAFFIC_H_

#include "Endpoint.h"


#include "src/templates/Vector.h"

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
	void initialize_client(DataIndication* msg);


	void send_event(uint32_t s);
	void printResult();

	uint32_t read_uint32_t(DataIndication* msg,int offset_in_byte = 0);

	void start(timeOffset_t offset);


	cometos::Vector<node_t,255> subs; // 255 elements is max for this vector


	static bool running;
	static unsigned int messagesSend;
	uint32_t last_sequencenumber;
	uint32_t amount_of_clients;


	                                   // Importent for |Coordinator|Client|
	unsigned long int counter;                // uint32 |   X       |      | counts events
	unsigned long int threshold;              // uint32 |   X       |      | truly a threshold
	unsigned long int distanceToCoordinator;  // uint32 |   X       |  X   | distance of Coordinator itself is 0 (obviously)
	timeOffset_t slf_msg_timer;               // uint16 |           |      | time to next action (right now not used)
	node_t out;                               // uint16 |           |  X   | gives a node the output for events (there is always just one valid output in the direction of coordinator)
	bool isSet;                               //        |   X       |  X   | true if out isSet           (Coordinator always true) (isSet has to be true for getting subs(see handleIndication))
	//pktSize_t payloadSize;                  // uint8  |           |      | also apparently not in usage right now


private:
	int sendCounter;

};


} /* namespace cometos */
#endif /* PERIODICTRAFFIC_H_ */
