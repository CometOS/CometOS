
#include <node_handler.h>
#include "Airframe.h"
#include "logging.h"

Define_Module(cometos::node_handler);

namespace cometos {

void node_handler::finish() {
	recordScalar("sendCounter", sendCounter);
}

bool node_handler::running=true;
unsigned int node_handler::messagesSend = 0;

void node_handler::initialize() {


    // dafuq DA steckt keine funktionalität drinne
	//Endpoint::initialize();

    isSet=false;
	sendCounter = 0;

	distanceToCoordinator=0;
	out = BROADCAST; // broadcast ist 2^16 was vorzeichen behaftet minus eins ist
	slf_msg_timer = 2000;


	CONFIG_NED(slf_msg_timer);
	CONFIG_NED(threshold);
	counter = 0;

	//was das ?
	//remoteDeclare(&node_handler::get, "get");


	  if (par("initiator"))    {
#ifdef OMNETPP
		start((static_cast<timeOffset_t>(par("start_delay"))));
#endif
}			}

void node_handler::start(timeOffset_t offset) {
	schedule(new Message(), &node_handler::flood_network, offset);
}

void node_handler::flood_network(Message *timer) {

        AirframePtr frame = make_checked<Airframe>();
        uint32_t welcome=1;
        (*frame) << welcome;
        sendRequest(new DataRequest(BROADCAST, frame, createCallback(&node_handler::resp)));

        //In case network should be flooded again
        //schedule(timer, &node_handler::flood_network, slf_msg_timer);
        //Since we don't take multiple floods in consideration, we delete msg.
        delete timer;



        //      Comment/LoG  SECTION
        LOG_INFO("wo wird dieser log ausgegeben ?!?!?!?! " << 42 << "\n");
        EV    << "debug test " << 42 << "\n";

}


void node_handler::generate_events(Message *timer) {
    if(node_handler::running){

       send_event();

    timeOffset_t offset = uniform(100, 1000);  // between 0.1 and one seconds
    schedule(timer, &node_handler::generate_events, offset);
    }
    else
    delete timer;
}


void node_handler::resp(DataResponse *response) {
	LOG_INFO("rcv rsp "<<(int)response->status);
	delete response;
}

void node_handler::handleIndication(DataIndication* msg) {
	LOG_INFO("recv "<<(int)msg->getAirframe().getLength()<<" bytes from "<<msg->src);

    if (par("initiator")) {
            if( (read_uint32_t(msg)==UINT32_MAX) ){ // just events should be count. Ignore broadcasts from coordinator itself

            counter++;

            if(counter>=threshold)
                {               // Printout messages
                                char buf[50];
                                sprintf(buf, "threshold reached with %d messages send overall",node_handler::messagesSend);
                                getDisplayString().setTagArg("t", 0, buf);
                                bubble(buf);
                                std::cout<<"threshhold reached with: "<<node_handler::messagesSend<<" messages over all"<<"\n"<<endl;
                    node_handler::running=false;
                }
            }
    }else{

        if(!isSet){
            isSet=true;

            // Set Clients knowledge about its place in the network
            distanceToCoordinator=read_uint32_t(msg);
            ASSERT(distanceToCoordinator != UINT32_MAX);  // the first message every node receives has to be a broadcast
            out = msg->src;

            // Send broadcast further (important in case of Multihop network)
            AirframePtr frame = make_checked<Airframe>();
            uint32_t welcome=distanceToCoordinator+1;
            (*frame) << welcome;
            sendRequest(new DataRequest(BROADCAST, frame, createCallback(&node_handler::resp)));


            // this node will start in at least 1000 ms generating events
            timeOffset_t offset = uniform(1000, 2000);  // between one and ten seconds
            schedule(new Message(), &node_handler::generate_events, offset);

        }else{

            // You are already set
            // Now distinguish bewteen broadcast of cordinator and an event of a node

            if( (read_uint32_t(msg) != UINT32_MAX) ){
             // its a broadcast
             // this node got the broadcast already earliear
            }else{
             // its an event, which needs to send on to the coordinator
                send_event();
            } // end of forward
        } // end of isSet if
    } // end of initiator if
    delete msg;
} // end of handleIndication


void node_handler::send_event(){
                    AirframePtr frame = make_checked<Airframe>();
                    uint32_t an_event=UINT32_MAX;
                    (*frame) << an_event;
                    sendRequest(new DataRequest(out, frame, createCallback(&node_handler::resp)));
                    sendCounter++;node_handler::messagesSend++;
                    WATCH(sendCounter);
                    WATCH(node_handler::messagesSend);
}

uint32_t node_handler::read_uint32_t(DataIndication* msg,int offset_in_byte){
                uint32_t z=0;
                uint8_t *pointer=msg->getAirframe().getData();
                for (uint8_t i = 0; i < (int)msg->getAirframe().getLength(); i++) {
                z +=  (pointer[i+offset_in_byte] << 8 *  (3-i) );
                }
                return z;
}


//StaticSList<uint8_t, TRAFFIC_MAX_PAYLOAD> node_handler::get(uint8_t& length,
//		uint8_t& start) {
//    StaticSList<uint8_t, TRAFFIC_MAX_PAYLOAD> ret;
//
//	for (uint8_t i = 0; i < length; i++) {
//		ret.push_back(start++);
//	}
//
//	return ret;
//
//}

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
