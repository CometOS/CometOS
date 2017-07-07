
#ifndef PERIODICTRAFFIC_H_
#define PERIODICTRAFFIC_H_

#include "Endpoint.h"
#include "Algo1.h"


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


	void send_event(uint32_t s,uint32_t status_code=2,uint32_t para = 0); // sends an event (goes in direction of coorinator)
	void send_new_threshold(uint32_t threshold,uint32_t status_code,uint32_t restart = 0); // distributes new threshold (goes from coordinator away)
	bool does_this_node_finished_these_round();

	void printResult();

	uint32_t read_uint32_t(DataIndication* msg,int offset_in_byte = 0);

	void start(timeOffset_t offset);
	void start_observing(Message *timer);


	cometos::Vector<node_t,255> subs; // 255 elements is max for this vector


	static bool running;
	static unsigned int messagesSend;
	static unsigned int messages_sendCount;
	static unsigned int messages_thresholdreached;
	static unsigned int messages_newRound;
	static unsigned int debug_val;
	uint32_t last_sequencenumber;
	uint32_t amount_of_clients;


	Algo* logic;

	                             // Importent/InUse for |Coordinator|Client|
	unsigned long int counter;                // uint32 |   X       |  X   | counts events (in second approach clients also use it)
	unsigned long int threshold;              // uint32 |   X       |      | truly a threshold
	unsigned long int distanceToCoordinator;  // uint32 |   X       |  X   | distance of Coordinator itself is 0 (obviously)
	node_t out;                               // uint16 |           |  X   | gives a node the output for events (there is always just one valid output in the direction of coordinator)
	bool isSet;                               //        |   X       |  X   | true if out isSet           (Coordinator always true) (isSet has to be true for getting subs(see handleIndication))

	bool childs_local_threshold_reached_one;  //        |   X       |  X   | Every Node can have child nodes (not more in algo1)
	unsigned long int last_threshold_send;
	unsigned long int temp_count;


	// fancy types
	// timeOffset_t slf_msg_timer uint16
	// pktSize_t    payloadSize   uint8


private:
	int sendCounter;
	int forwardCounter;

};


} /* namespace cometos */
#endif /* PERIODICTRAFFIC_H_ */
