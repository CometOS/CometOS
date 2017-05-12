#include <threshold_monitoring/node_handler.h>
#include "Airframe.h"
#include "logging.h"

Define_Module(cometos::node_handler);

namespace cometos {

void node_handler::finish() {
    recordScalar("sendCounter", sendCounter);
}

bool node_handler::running = true;
unsigned int node_handler::messagesSend = 0;

void node_handler::initialize() {

    Endpoint::initialize();

    isSet = false;
    sendCounter = 0;
    last_sequencenumber = 0;
    amount_of_clients = par("num_clients");

    out = BROADCAST; // broadcast ist 2^16 was vorzeichen behaftet minus eins ist
    slf_msg_timer = 2000;

    CONFIG_NED(slf_msg_timer);
    CONFIG_NED(threshold);
    counter = 0;

    //remoteDeclare(&node_handler::get, "get");

    if (par("initiator")) {
        isSet = true; // damit der koordinator später auch seine subs bekommt
        distanceToCoordinator = 0;
#ifdef OMNETPP
        start((static_cast<timeOffset_t>(par("start_delay"))));
#endif
    }
}

void node_handler::start(timeOffset_t offset) {
    schedule(new Message(), &node_handler::flood_network, offset);
}

void node_handler::flood_network(Message *timer) {
    delete timer;

    AirframePtr frame = make_checked<Airframe>();
       uint32_t status_code = 1; // 1 = "initialization broadcast"
       uint32_t dist        = 1; // neighbour of coordinator have a distance of 1
       (*frame) << status_code;
       (*frame) << dist;
    sendRequest(new DataRequest(BROADCAST, frame,createCallback(&node_handler::resp)));

    //      Comment/LoG  SECTION
    LOG_INFO("wo wird dieser log ausgegeben ?!?!?!?! " << 42 << "\n");
    EV_ERROR<< "debug test " << 42 << "\n";

}

void node_handler::generate_events(Message *timer) {
    if (node_handler::running) {

        EV <<"node "<< palId_id()<<" is generating one event"<< endl;
        send_event(palId_id()+sendCounter * amount_of_clients);

        timeOffset_t offset = uniform(600, 1000); // between 0.1 and one seconds
        schedule(timer, &node_handler::generate_events, offset);
    } else
        delete timer;
}

void node_handler::resp(DataResponse *response) {
    LOG_INFO("rcv rsp "<<(int)response->status);
    EV << "rcv rsp " << (int) response->status << " from " << response->getId()<<" (bei Broadcast wennn rausgeschickt wurde, ansonsten bei ACK) "<<endl;
    delete response;
}

void node_handler::handleIndication(DataIndication* msg) {
    LOG_INFO("recv "<<(int)msg->getAirframe().getLength()<<" bytes from "<<msg->src);

   switch ( (int)read_uint32_t(msg) ) // Read Statuscode
   {
   case 1 :  // 1 = "initialization broadcast"
           if (par("initiator")) break; // Coordinator doesn't care
           if (!isSet) {
                isSet=true;
                initialize_client(msg);
                       }
           break;
   case 2 :  // 2 = "count"
           if(last_sequencenumber != read_uint32_t(msg,4))
           {
           if (par("initiator")) {
                       counter++;
                       EV << "counter increased to "<<counter<<endl;
                       EV << "messages send here: "<<node_handler::messagesSend<<endl;
                       if (counter >= threshold) {
                       printResult();
                       node_handler::running = false;
                       }
           }else
           {
               ASSERT(isSet); // you can just get a count if u are Set
               send_event(read_uint32_t(msg,4));
           }
           last_sequencenumber=read_uint32_t(msg,4);
           }
           else
           {
               EV_ERROR << "Same message again: probably lost of an acknowledgement"<<endl;
           }
           break;
   case 3 : // 3 = "here I am message"
           ASSERT(isSet); // you can just get a sub message if u are Set
           subs.pushBack(msg->src);
           {uint32_t size = subs.getSize();EV << "Size of subs of node " << palId_id() << " is now " << size << endl;} // Debug message
           break;
   case 4 : // 4 = "Update" distribute new threshold or somethin (for round based algorithms)
           ASSERT(isSet); // you can just get an update if u are Set

           {
           uint32_t size = subs.getSize();
           for(int i=0;i!=size;i++)
           {
           subs[i]; // TO DO
           }
           } // end of scope
           break;
   case 5 : // 5 = "local threshold reached" inform coordinator
           ASSERT(isSet);

           if (par("initiator")) {
           // TO DO
           //    DO SOME LOGIC
           }else{
               // weiteleiten richtung coordinator
           }
           break;
   default  :  EV_ERROR <<"Error unknown message: "<< read_uint32_t(msg) <<endl; break;
   }
    delete msg;
} // end of handleIndication


void node_handler::initialize_client(DataIndication* msg){

    // Set Clients knowledge about its place in the network
    distanceToCoordinator = read_uint32_t(msg,4);
    WATCH(distanceToCoordinator);
    out = msg->src;

    // Send broadcast further (important in case of Multihop network)
    AirframePtr frameBRO = make_checked<Airframe>();
    uint32_t status_code = 1; // 1 = "initialization broadcast"
    uint32_t dist = distanceToCoordinator+1;
    (*frameBRO) << status_code;
    (*frameBRO) << dist;
    sendRequest(new DataRequest(BROADCAST, frameBRO,createCallback(&node_handler::resp)));
    EV << "sending broadcast further" << endl;

    // Send an "here i am", so that the upper node know you
    AirframePtr frameACK = make_checked<Airframe>();
    status_code = 3;  // 3 = "here I am message"
    (*frameACK) << status_code;
    sendRequest(new DataRequest(out, frameACK,createCallback(&node_handler::resp)));
    EV << "sending here i am message" << endl;

    // this node will start in at least 1000 ms generating events
    timeOffset_t offset = uniform(1000, 2000); // between one and two seconds
    schedule(new Message(), &node_handler::generate_events, offset);

}


void node_handler::send_event(uint32_t s) {
    EV << "sending event" << endl;
    AirframePtr frame = make_checked<Airframe>();
    uint32_t status_code = 2; // 2 = "count"
    uint32_t sequence =  s+amount_of_clients ; // sequencenumber is importent in case an acknowledgement get lost
    (sequence >= (UINT32_MAX-amount_of_clients)) ? (EV_WARN<<"sequence_number_warap_around"<<endl) : (EV<<"sequence_number_are_ok"<<endl);
    (*frame) << status_code;
    (*frame) << sequence;
    sendRequest(new DataRequest(out, frame, createCallback(&node_handler::resp)));
    sendCounter++;
    node_handler::messagesSend++;
    WATCH(sendCounter);
    WATCH(node_handler::messagesSend);
}

void node_handler::printResult(){
    char buf[50];
    sprintf(buf, "threshold reached with %d messages send overall",node_handler::messagesSend);
    getDisplayString().setTagArg("t", 0, buf);
    bubble(buf);
    std::cout << "threshhold reached with: "<< node_handler::messagesSend << " messages over all"<< "\n" << endl;
}

uint32_t node_handler::read_uint32_t(DataIndication* msg, int offset_in_byte) {
    uint32_t z = 0;
    uint8_t *pointer = msg->getAirframe().getData();
    for (uint8_t i = 0; i < 4; i++) { //(int)msg->getAirframe().getLength()
        z += (pointer[i + offset_in_byte] << 8 * (3 - i));
    }
    return z;
}

} /* namespace cometos */
