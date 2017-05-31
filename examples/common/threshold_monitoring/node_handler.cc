#include <threshold_monitoring/node_handler.h>
#include "Airframe.h"
#include "logging.h"


Define_Module(cometos::node_handler);

namespace cometos {

void node_handler::finish() {
    recordScalar("sendCounter", sendCounter);
    delete logic;
}

bool node_handler::running = true;
unsigned int node_handler::messagesSend = 0;

void node_handler::initialize() {

    Endpoint::initialize();

    isSet     = false;
    hasStarted= false;
    childs_local_threshold_reached_one=false;


    sendCounter = 0;
    forwardCounter=0;
    last_sequencenumber = 0;
    amount_of_clients = par("num_clients");

    out = BROADCAST; // broadcast ist 2^16 was vorzeichen behaftet minus eins ist

    CONFIG_NED(threshold);
    counter = 0;



    logic=new Algo1();

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

void node_handler::start_observing(Message *timer) {
    delete timer;


    logic->set_k((int)subs.getSize());
    logic->set_local_Slack(threshold); // local Slack from Coordinator is overall Slack
    uint32_t local_threshold = logic->calc_local_threshold();


    EV<<"threshold = "<<threshold<<endl;
    EV<<"k = "<<(int)subs.getSize()<<endl;
    EV<<"local_threshold ="<<local_threshold<<endl;



    uint32_t status_code = 6;
    uint32_t size = subs.getSize();
    for(unsigned int i=0;i!=size;i++)
    {
        AirframePtr frame = make_checked<Airframe>();
        (*frame) << status_code;
        (*frame) << local_threshold;
        sendRequest(new DataRequest(subs[i], frame,createCallback(&node_handler::resp)));
    }

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


    timeOffset_t offset = 2000; // 2 Seconds untill start obersving
    schedule(new Message(), &node_handler::start_observing, offset);
}

void node_handler::generate_events(Message *timer) {
    if (node_handler::running) {

        EV <<"node "<< palId_id()<<" is generating one event"<< endl;
        //send_event(palId_id()+sendCounter * amount_of_clients);

        //if(palId_id()==6){
        if(logic->count()){
            if(logic->get_local_threshold()<=1)
            {
            send_event(palId_id()+sendCounter * amount_of_clients);
            EV << "sendet single events " << "\n";
            }
            else
            {
                EV << "sendet threshold_reached of "<< logic->get_local_threshold() << "\n";
            // Update - Send a "local_threshold_reached()"
            uint32_t local_threshold_reached=5;
            uint32_t local_threshold_was = logic->get_local_threshold();
            send_event(palId_id()+sendCounter * amount_of_clients,local_threshold_reached,local_threshold_was);
            }
        }
        //} // end of id=1

        timeOffset_t offset = uniform(1000, 2000); // between one and two seconds
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
           {
           bool is=false;
           uint32_t size = subs.getSize();
           for(unsigned int i=0;i!=size;i++)
           { if(subs[i]==msg->src)is=true;}

           if(is==false)
           {
           subs.pushBack(msg->src);

           EV<<msg->src<<" ist knecht von "<<palId_id()<<endl;
           {uint32_t size = subs.getSize();EV << "Size of subs of node " << palId_id() << " is now " << size << endl;} // Debug message
           }
           else
           {EV<<msg->src<<" ist bereits knecht von "<<palId_id()<<endl;}
           }
           break;
   case 4 : // 4 = "new_Round()" distribute new threshold or somethin (for round based algorithms)


       EV<<"MY slack is "<<read_uint32_t(msg,4)<<endl;
       EV<<"MY k is "<<logic->get_k()<<endl;
                     EV<<"MY threshold is "<<logic->get_local_threshold()<<endl;


           ASSERT(isSet); // you can just get an update if u are Set
           ASSERT(!par("initiator")); // the coordinator never gets a new_Round message !
           ASSERT(logic->get_local_threshold()!=0); // if threshold reached zero, there is somethin fishy
           //ASSERT(logic->get_local_threshold()!=1); // without  Multihop this ASSERT is TRUE

           if(logic->get_local_threshold()!=1){
           {







           if(logic->new_Round(read_uint32_t(msg,4))){
               // An Update results in a new reached threshold
               EV << "sendet threshold_reached of "<<read_uint32_t(msg,4)<<" (due to new round)"<< "\n";

                           uint32_t local_threshold_reached=5;
                           uint32_t                   count=2;

                           if(read_uint32_t(msg,4)<=1)
                           send_event(palId_id()+sendCounter * amount_of_clients,count);
                           else
                           send_event(palId_id()+sendCounter * amount_of_clients,local_threshold_reached,read_uint32_t(msg,4));
           }


           EV << "runde: "<< logic->get_round()<< " | local_count: "<< logic->get_count() <<"\n";



           if(logic->get_k()==1)ASSERT(logic->get_local_threshold()==logic->get_Slack());


           uint32_t size = subs.getSize();
           uint32_t status_code=4;


               EV<<"my slack is "<<read_uint32_t(msg,4)<<endl;
               EV<<"my threshold is "<<logic->get_local_threshold()<<endl;




           for(unsigned int i=0;i!=size;i++)
           {
               AirframePtr frame = make_checked<Airframe>();
                         (*frame) << status_code;
                         (*frame) <<(uint32_t)logic->get_local_threshold();
               sendRequest(new DataRequest(subs[i], frame, createCallback(&node_handler::resp)));
               forwardCounter++;
               node_handler::messagesSend++;
               EV<<"new Round weitergeleitet (messages: "<<node_handler::messagesSend<<")\n";
           }

           }

           } // end of scope
           break;
   case 5 : // 5 = "local_threshold_reached()" inform coordinator
           ASSERT(isSet);

           if (par("initiator")) {


               counter+=read_uint32_t(msg,8);
               EV << "Counter um folgenden threshold erhöht: "<<read_uint32_t(msg,8)<< "\n";


               EV << "Counter ist nun schon: "<<counter<< "\n";
               //EV << "Slack: "<<logic->get_Slack()<<" | k = "<<logic->get_k()<< "\n";

               if (counter >= threshold) {
               printResult();
               node_handler::running = false;
               }else if(childs_local_threshold_reached_one==false){



               logic->new_Round_c(read_uint32_t(msg,8));

               uint32_t new_local_threshold=logic->calc_local_threshold();



               EV << "neuer Slack/threshld ist "<<logic->get_Slack()<< "\n";
               EV << "neuer localer_threshold ist "<<new_local_threshold<< "\n";
               EV << "Coordinator veranlässt eine neue runde an alle: "<<logic->get_round()<< "\n";


               int size=(int) subs.getSize();
               uint32_t status_code=4;



                          for(int i=0;i!=(size);i++)
                          //for(int i=(size-1);i!=(-1);i--)
                          {
                            AirframePtr frame = make_checked<Airframe>();
                                      (*frame) << status_code;
                                      (*frame) << new_local_threshold;

                            sendRequest(new DataRequest(subs[i], frame,createCallback(&node_handler::resp)));
                            sendCounter++;
                            node_handler::messagesSend++;
                            EV << "neue runde an "<<subs[i]<<" (messages: "<<node_handler::messagesSend<< ")\n";

                          }

                          if(new_local_threshold == 1)childs_local_threshold_reached_one=true;


               }



           }else{
               // weiteleiten richtung coordinator
               send_event(read_uint32_t(msg,4),5,read_uint32_t(msg,8));
           }
           break;
   case 6 : // 6 = "start observing" coordinator informs clients that they can start observing events
            if ((!par("initiator")) && hasStarted==false){
                hasStarted=true;

                uint32_t size = (int)subs.getSize();
                unsigned long int k = size ;
                EV<<"k is"<<k<<endl;
                k = k + 1;
                EV<<"and now "<<k<<endl;

                logic->set_k(k); // here the client himself is in k !!!
                logic->set_local_Slack(read_uint32_t(msg,4));
                uint32_t local_threshold = logic->calc_local_threshold();
                logic->set_local_treshold(local_threshold);

               // this node will start in at least 1000 ms generating events
               timeOffset_t offset = uniform(1000, 2000); // between one and two seconds
               schedule(new Message(), &node_handler::generate_events, offset);

               // Send broadcast further (important in case of Multihop network)



               uint32_t status_code = 6;
               for(unsigned int i=0;i!=size;i++)
               {
                   AirframePtr frameBRO = make_checked<Airframe>();
                   (*frameBRO) << status_code;
                   (*frameBRO) << local_threshold;
                   sendRequest(new DataRequest(subs[i], frameBRO,createCallback(&node_handler::resp)));
                   EV << "sending start observing further to"<<subs[i]<< endl;
               }


//                 BROADCASTS gehen immernoch mal verloren, warum auch immer
//               AirframePtr frameBRO = make_checked<Airframe>();
//               (*frameBRO) << status_code;
//               (*frameBRO) << local_threshold;
//               sendRequest(new DataRequest(BROADCAST, frameBRO,createCallback(&node_handler::resp)));
//               // mit dem kann auch der hasStarted kram gelöscht werden


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


    // Debug Output for Algorithm
    EV << "(runde: "<< logic->get_round()<< ") | local_count"<< logic->get_count() <<"\n";

}


void node_handler::send_event(uint32_t s,uint32_t status_code,uint32_t para) {
    EV << "sending event"<<(int)status_code<< endl;
    AirframePtr frame = make_checked<Airframe>();

    (*frame) << status_code; // default status_code = 2

    uint32_t sequence =  s+amount_of_clients ; // sequencenumber is importent in case an acknowledgement get lost
    (sequence >= (UINT32_MAX-amount_of_clients)) ? (EV_WARN<<"sequence_number_warap_around"<<endl) : (EV<<"sequence_number_are_ok"<<endl);
    (*frame) << sequence;


    if(status_code==5) (*frame) << para;

    sendRequest(new DataRequest(out, frame, createCallback(&node_handler::resp)));
    sendCounter++;
    node_handler::messagesSend++;
    EV << "Send_Event: Messages " <<node_handler::messagesSend<< endl;
    WATCH(sendCounter);
    WATCH(node_handler::messagesSend);
}

void node_handler::printResult(){
    char buf[50];
    sprintf(buf, "threshold reached with %d messages send overall",node_handler::messagesSend);
    getDisplayString().setTagArg("t", 0, buf);
    bubble(buf);
    std::cout << "counter = "<< counter  << endl;
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
