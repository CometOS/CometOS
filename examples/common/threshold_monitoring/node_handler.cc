#include <threshold_monitoring/node_handler.h>
#include "Airframe.h"
#include "logging.h"

// erster include nicht notwendig , 2ter schon wtf
#include <threshold_monitoring/Algo1.h>
#include <threshold_monitoring/Algo2.h>


#include <fstream>
#include <stdio.h>
#include <ctime>
#include <cstdlib>
#include <stdlib.h>

Define_Module(cometos::node_handler);

namespace cometos {

void node_handler::finish() {
    recordScalar("sendCounter", sendCounter);
    delete logic;
}

bool node_handler::running = true;
unsigned int node_handler::messagesSend = 0;
unsigned int node_handler::debug_val= 0;
unsigned int node_handler::messages_sendCount= 0;
unsigned int node_handler::messages_thresholdreached= 0;
unsigned int node_handler::messages_newRound= 0;

void node_handler::initialize() {
//    std::ofstream file;
//    file.open("results//random");
//    double rand = uniform(0, 10);
//
//    file << rand << std::endl;
//
//    file.close();

    Endpoint::initialize();

    isSet     = false;


    childs_local_threshold_reached_one=false;

    last_threshold_send=0;
    sendCounter = 0;
    forwardCounter=0;
    last_sequencenumber = 0;
    temp_count = 0;
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

    logic->set_local_branch_threshold(threshold);
    logic->set_k((int)subs.getSize());
    logic->set_local_Slack(threshold); // local Slack from Coordinator is overall Slack
    uint32_t local_threshold = logic->calc_local_threshold();

    EV<<"threshold = "<<threshold<<endl;
    EV<<"k = "<<(int)subs.getSize()<<endl;
    EV<<"local_threshold ="<<local_threshold<<endl;

    send_new_threshold(local_threshold,6);

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
        ASSERT(logic->get_local_threshold()>0);


        if(logic->get_Slack()==0){
            EV<<"Warning: event observed while waiting"<<endl;
        }else
        {

       // if(palId_id()==2 ) {

          // trivialer algo
          //  send_event(palId_id()+sendCounter * amount_of_clients);


        EV <<"node "<< palId_id()<<" is generating one event"<< endl;
        EV <<"threshold is "<< logic->get_local_threshold()<< endl;
        EV <<"b = "<<logic->get_local_branch_threshold()<<endl;
        EV <<"Slack = "<<logic->get_Slack()<<endl;
        EV <<"local_count (noch ohne +1)= "<<logic->get_count()<<endl;
        EV <<"counter = "<<counter<<endl;




        if(logic->count()){
            if(logic->get_local_threshold()==1)
            {
                if(logic->get_k()==1) {
                    send_event(palId_id()+sendCounter * amount_of_clients);        EV << "sendet single events " << "\n";
                }else{
                    counter++;                                                     EV <<"counter is "<<counter<<endl;
                    if(logic->isAlgo1()){logic->set_local_Slack(logic->get_Slack()-1);}
                    does_this_node_finished_these_round();   // check if new round and react properly
                }
            }
            else
            {

                if(logic->get_k()==1) { // Node ist kein Koordinator

                EV << "sendet threshold_reached of "<< logic->get_local_threshold() << "\n";
            if(logic->isAlgo1())  send_event(palId_id()+sendCounter * amount_of_clients,5,logic->get_local_threshold());
            else                  send_event(palId_id()+sendCounter * amount_of_clients,7,logic->get_local_threshold());

                }else{ // Wenn der Node selbst Koordinator ist
                    EV << "reached threshold of "<< logic->get_local_threshold() << "\n";

                    logic->run_var++;
                    counter+=logic->get_local_threshold();

                    if(logic->run_var==logic->get_k() && (!logic->isAlgo1())){
                        ASSERT(counter<logic->get_local_branch_threshold()); // der ast schwellenwert kann nicht über local threshold ungleich eins erreicht werden
                        EV <<logic->run_var<<" times were this reached "<< logic->get_local_threshold() << "\n";
                        logic->new_Round_c(0);                                                // increment round
                        logic->set_local_treshold(logic->calc_local_threshold());             // set new threshold
                        send_new_threshold(logic->get_local_threshold(),8,0);                     // distribute new Slack/threshold
                        EV <<"new threshold is: "<<logic->get_local_threshold()<< "\n";
                        EV <<"count is "<<counter<<endl;
                        logic->run_var=0;
                    }


                    if(logic->isAlgo1()){
                        EV<<" logic threshold reached was "<<logic->get_local_threshold()<<endl;
                        EV<<" k is "<<logic->get_k()<<endl;
                        EV<<" Slack was "<<logic->get_Slack()<<endl;

                        logic->new_Round_c(logic->get_local_threshold());                    EV<<" Slack is now "<<logic->get_Slack()<<endl;
                        uint32_t new_local_threshold=logic->calc_local_threshold();

                        if(does_this_node_finished_these_round()){                     // hier wird counter ja wieder gesenkt
                               // ASSERT(logic->get_Slack()==0); Slack nun kaputt seien, durch zu hohen threshold (siehe degnerierter fall 2)
                                logic->set_local_Slack(0); // setzte 0 falls er nicht schon ist
                                EV<<" Eine neue Runde, durch erreichen eines thresholds > 1"<<endl;
                         }else{ ASSERT(logic->get_Slack()!=0);
                         logic->set_local_treshold(new_local_threshold);                     EV<<" threshold new "<<logic->get_local_threshold()<<endl;
                         send_new_threshold(new_local_threshold,4);
                            }
                    }




                }


            }
        }



       // } // end of id

        } // end of Slack==0

        timeOffset_t offset = uniform(1000, 2000); // between one and two seconds

      //  if(palId_id()==2) offset/=3;

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

   // logic->handle_Message(read_uint32_t(msg), msg);

   switch ( (int)read_uint32_t(msg) ) // Read Statuscode
   {
   case 1 :  // 1 = "initialization broadcast"
           if (par("initiator")) break; // Coordinator doesn't care
           if (!isSet) {  EV<<"my id is "<<palId_id()<<endl; // ASSERT(palId_id()<=amount_of_clients);
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

                       if(logic->isAlgo1())logic->new_Round_c(1); // reduziere Slack

                       uint32_t new_local_threshold=logic->calc_local_threshold();

                       EV<<"Slack = "<<logic->get_Slack()<<" und k = "<<logic->get_k() <<" und count = "<<counter<<" und runde= "<<logic->get_round()<<endl;

                       if(counter< threshold && logic->isAlgo1())ASSERT(new_local_threshold!=0); // Slack ist nicht 0 und dadurch auch nicht new_local_threshold
                       if(counter==threshold && logic->isAlgo1())ASSERT(new_local_threshold==0); // in der letzten runde ist Slack 0 und dadurch auch new_local_threshold

                       EV<<"new_local_threshold is: "<<new_local_threshold<<endl;
                       if(new_local_threshold!=1 && logic->isAlgo1() && counter<threshold)
                       { send_new_threshold(new_local_threshold,4); } // ist möglich wenn am ende mehrere counts due to new round gesendet werden (e.g. t=10000 seed=5 concentric_19)

           }else
           {
//               send_event(read_uint32_t(msg,4)); // trivialer algo
               EV<<"temp_count"<<temp_count<<endl;
               if(logic->get_local_branch_threshold()==1 && temp_count==0) // temp_count betrifft nur algo1
               send_event(read_uint32_t(msg,4));
               else{

               if(!logic->isAlgo1()){
               counter++;                        EV<<"counter is: "<<counter<<" (count from "<<msg->src<<"), I may need to wait (b = "<<logic->get_local_branch_threshold()<<")"<<endl;
               }
               if(logic->isAlgo1()){if(logic->get_Slack()==0){counter++;}
               else {counter++; logic->set_local_Slack(logic->get_Slack()-1);}}

               does_this_node_finished_these_round(); // check if new round and react properly
               }
           }
           last_sequencenumber=read_uint32_t(msg,4);
           } // end of sequencenumber
           else
           {
               EV_ERROR << "Same message again: probably lost of an acknowledgement "<<read_uint32_t(msg,4)<<endl;
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
           {// if(msg->src==27) ASSERT(palId_id()!=20);
           subs.pushBack(msg->src);           EV<<msg->src<<" hängt an   " << palId_id() <<endl;
           {uint32_t size = subs.getSize();   EV<< "Size of subs of node " << palId_id() << " is now " << size << endl;} // Debug message
           }
           else                              {EV<<msg->src<<" hängt bereits an "<<palId_id()<<endl;}
           }
           break;
   case 4 : // 4 = "new_Round()" distribute new threshold or somethin (for round based algorithms)
   {
       EV<<"MY new Slack is "<<read_uint32_t(msg,4)<<endl;
       EV<<"MY local count is "<<logic->get_count()<<endl;
       EV<<"My local counter is"<<counter<<endl;


           ASSERT(read_uint32_t(msg,4)!=0);
           ASSERT(isSet); // you can just get an update if u are Set
           ASSERT(!par("initiator")); // the coordinator never gets a new_Round message !
           ASSERT(logic->get_local_threshold()!=0); // if threshold reached zero, there is somethin fishy
           //ASSERT(logic->get_local_threshold()!=1); // without  Multihop this ASSERT is TRUE
           if(logic->get_Slack()==0) ASSERT(logic->get_k()!=1); // Slack kann 0 sein, wenn dieser Node grade eben selbst den lokalen ast threshold erreicht hat durch andere (siehe 5)

           logic->set_local_branch_threshold(read_uint32_t(msg,4));
           logic->set_local_Slack(read_uint32_t(msg,4));
           EV <<"b = "<<logic->get_local_branch_threshold()<<endl;



           bool its_new_round=false; // true = local_count wurde zum counter addiert und fühert zu einer neuen runde


            if(does_this_node_finished_these_round()){
                // warte auf neue new_Round()
                EV<<"Slack wurde sofort überschritten, warte auf neuen Slack"<<endl;
                logic->set_local_Slack(0);
            }else{

                ASSERT(counter<logic->get_Slack());
                logic->new_Round_c(counter); // reduziere slack
                temp_count = counter;
                counter=0;                  // mit max besprochen, dass counter vom slack direkt abgzogen wird
                logic->set_local_branch_threshold(logic->get_Slack());


           while(logic->new_Round(read_uint32_t(msg,4))){ // berechne neuen threshold und checke ob erreicht
             //  if(logic->new_Round(read_uint32_t(msg,4))){
               // An Update results in a new reached threshold
               EV<<"MY new threshold is "<<logic->get_local_threshold()<<endl;

               if(logic->get_k()==1){
               EV << "sendet threshold_reached of "<<read_uint32_t(msg,4)<<" (due to new round)"<< "\n";

                           uint32_t                   count=2;
                           uint32_t local_threshold_reached=5;

                           if(logic->get_local_threshold()==1){ ASSERT(logic->get_Slack()==1);
                                                                                     send_event(palId_id()+sendCounter * amount_of_clients,count);
                              while(logic->get_count()!=0){debug_val++; logic->decrement_count();send_event(palId_id()+sendCounter * amount_of_clients,count);} // theorettisch optimierungspotenzial, die counts alle in eine nachricht zu packen, aber nö
                           }
                           else
                           send_event(palId_id()+sendCounter * amount_of_clients,local_threshold_reached,logic->get_local_threshold());
               }
               else{
                       logic->new_Round_c(logic->get_local_threshold()); // reduziere slack
                       counter+=logic->get_local_threshold();
                       EV<<" Slack is reduced to"<<logic->get_Slack()<<endl;


                       // checken ob hier durch der lokale ast threshold überschritten wurde
                       if(logic->get_Slack()==0){
                           ASSERT(counter==logic->get_local_branch_threshold());
                           does_this_node_finished_these_round();
                           its_new_round=true;
                           break;
                       }
//                       else
//                       {
//                        uint32_t new_local_threshold=logic->calc_local_threshold();
//                        logic->set_local_treshold(new_local_threshold);
//                       }
                        EV<<" threshold "<<logic->get_local_threshold()<<endl;
                        EV<<" local_count "<<logic->get_count()<<endl;

               }
           }

           EV << "runde: "<< logic->get_round()<< " | local_count: "<< logic->get_count() <<"\n";
           EV<<"  MY threshold is "<<logic->get_local_threshold()<<endl;
           EV<<"  MY branch threshold is "<<logic->get_local_branch_threshold()<<endl;


           if(logic->get_k()==1)ASSERT(logic->get_local_threshold()==logic->get_Slack());



            // Distribute new Slack/threshold further
          // if(its_new_round==false){
               EV<<" Counter hat noch nicht b erreicht darum neue runde, weiter leiten"<<endl;
               EV<<" Counter: "<<counter<<" | local_count: "<<logic->get_count()<<" | Slack: "<<logic->get_Slack()<<" | threshold: "<<logic->get_local_threshold()<<endl;
               send_new_threshold((uint32_t)logic->get_local_threshold(),4);}
           // }



   }
           break;
   case 5 : // 5 = "local_threshold_reached()"   Algo1


       if(last_sequencenumber != read_uint32_t(msg,4))
       {

           ASSERT(isSet);
           //if(logic->get_k()!=1)

           EV<<"Nachricht von "<<msg->src<< "("<<read_uint32_t(msg,8)<<")"<<endl;
           EV<<" MY Slack "<<logic->get_Slack()<<endl;
           EV<<" MY local count "<<logic->get_count()<<endl;
           EV<<" MY counter "<<counter<<endl;
           EV<<" MY b "<<logic->get_local_branch_threshold()<<endl;


           if(logic->get_Slack()==0 && (!par("initiator"))){ // vorher war Slack==0
               //ASSERT(counter==0); // muss nicht unbedingt sein , wenn er z.b. mehrere nachrichten in dem status bekommt !
               counter+=read_uint32_t(msg,8);
               ASSERT(((bool)par("initiator"))==false); // der koordinator kann & darf hier nicht rein
           }else{ if(logic->get_Slack()<=read_uint32_t(msg,8) && (!par("initiator"))){

             //  counter+=logic->get_Slack();  // addiere slack auf counter
             //  temp_count+=(read_uint32_t(msg,8)-logic->get_Slack()); // speicher den rest des counts
              // alternativ temp count nach counter reduzierten (assert(does this node finisched_round))

               counter+=read_uint32_t(msg,8);
               logic->set_local_Slack(0);    // setzte  slack 0

               ASSERT(does_this_node_finished_these_round());
               EV<<" heyho "<<endl;



           }else{

           ASSERT(logic->get_Slack()+counter==logic->get_local_branch_threshold());

              counter+=read_uint32_t(msg,8);            // auf counter addieren
              logic->new_Round_c(read_uint32_t(msg,8)); // & vom Slack abziehen
              EV << "Counter um folgenden threshold erhöht: "<<read_uint32_t(msg,8)<< "\n";
              EV << "Counter ist nun schon: "<<counter<< "\n";
              EV << "Slack ist nun schon: "<<logic->get_Slack()<<endl;








              if (counter >= threshold) {
              printResult();
              node_handler::running = false;
              //ASSERT(false); // threshold darf nur über counts erreicht werden, praktisch kommen nachrichten verzögert an übertreffen den threshold trozdem
              }else
              {


                     bool reached_new_round_due_to_count_from_coordinator=false;

                    if(does_this_node_finished_these_round()){
                         ASSERT(logic->get_Slack()==0);
                         reached_new_round_due_to_count_from_coordinator=true;
                     }else
                     {



// Beschreibung: Wenn ein Knoten einen lokalen schwellenwert geschickt bekommt, kann der lokale schwellenwert gesenkt und durch den koorindator knoten sofort erreicht werden
//               Ist das der Fall ist  reached_new_round_due_to_count_from_coordinator = true

               while((logic->new_Round(read_uint32_t(msg,4))==true) && (!par("initiator"))){  // neuen threshold berechnen und schauen ob drüber und ggf. vom coutn abziehen
                   // if((logic->new_Round(read_uint32_t(msg,4))==true) && (!par("initiator"))){
                     counter+=logic->get_local_threshold();            // counter erhöhen
                     logic->new_Round_c(logic->get_local_threshold()); // Slack senken
                          EV<<" Slack is reduced to "<<logic->get_Slack()<<" due to the count from the coordinator knoten"<<endl;
                          //logic->set_local_treshold(logic->calc_local_threshold()); // (falls while wieder rein, muss das raus)

                          if(logic->get_Slack()==0){ // wenn hier durch der lokale ast threshold überschritten wurde
                              EV<<"b "<<logic->get_local_branch_threshold()<<endl;
                              EV<<"counter "<<counter<<endl;

                              ASSERT(counter==logic->get_local_branch_threshold());
                              does_this_node_finished_these_round();
                              EV<<"b wurde erreicht"<<endl;
                              reached_new_round_due_to_count_from_coordinator=true;
                              logic->set_local_treshold(1); // nur temporär solange bis wir eine antwort vom parent bekommen
                              break;
                          }
               } // end of while




               uint32_t new_local_threshold=logic->get_local_threshold();


               EV << "neuer Slack/threshld ist "<<logic->get_Slack()<< "\n";
               EV << "neuer localer_threshold ist "<<new_local_threshold<< "\n";





              // if(reached_new_round_due_to_count_from_coordinator==false){
               send_new_threshold(new_local_threshold,4); // Nachricht 5 führt immer zu einem UPdate !! solang welche da sind
               if(distanceToCoordinator==0) EV << "Coordinator veranlässt eine neue runde an alle: "<<logic->get_round()<< "\n";
               else                         EV << "jemand veranlässt eine neue runde an alle: "<<logic->get_round()<< "\n";
              // }

                     } // end of, falls counter direkt überschritten war
                  } // end of, falls threshold rerriecht
           }}

           last_sequencenumber=read_uint32_t(msg,4);
       } // end of sequencenumber
       else
       {
           EV_ERROR << "Same message again: probably lost of an acknowledgement "<<read_uint32_t(msg,4)<<endl;
       }
           break;
   case 6 : // 6 = "start observing" coordinator informs clients that they can start observing events
            if (!par("initiator")){


                uint32_t size = (int)subs.getSize();
                unsigned long int k = size ;
                EV<<"k is"<<k<<endl;
                k = k + 1;
                EV<<"and now "<<k<<endl;

                logic->set_k(k); // here the client himself is in k !!!
                logic->set_local_branch_threshold(read_uint32_t(msg,4));
                logic->set_local_Slack(read_uint32_t(msg,4));
                uint32_t local_threshold = logic->calc_local_threshold();
                if((!logic->isAlgo1()) && logic->get_k()==1) {local_threshold=logic->get_Slack();}
                logic->set_local_treshold(local_threshold);



                EV<<"my initial Slack is "<<logic->get_Slack()<<endl;
                EV<<"my initial threshold is "<<local_threshold<<endl;

               // this node will start in at least 1000 ms generating events
               timeOffset_t offset = uniform(1000, 2000); // between one and two seconds
               schedule(new Message(), &node_handler::generate_events, offset);

               // Send broadcast further (important in case of Multihop network)
               send_new_threshold(local_threshold,6);

           }

           break;
   case 7 : // 7 = "local_threshold_reached"   Algo2
       ASSERT(isSet);

{
               counter+=read_uint32_t(msg,8);

               if (counter >= threshold) {
                                    printResult();
                                    node_handler::running = false;
                                    }else{


               if(logic->get_Slack()!=0){




           if(counter>logic->get_local_branch_threshold()){ // nicht zu verhindern

               //ASSERT(false);

               ASSERT(does_this_node_finished_these_round()==true);



           }else{







                      EV << "Counter ist: "<<counter<< "\n";
                      EV << "Counter um folgenden threshold erhöht: "<<read_uint32_t(msg,8)<< " von "<<msg->src<< "\n";
                      ASSERT(counter<=logic->get_local_branch_threshold());
                      EV << "Counter ist nun schon: "<<counter<< "\n";




                      if(childs_local_threshold_reached_one==false){

                          logic->run_var++;
                          EV << "run_var is "<<logic->run_var<<endl;

                          if(logic->run_var==logic->get_k())
                          {  logic->run_var=0;



                      // berechne neuen local_threshold mit selben slack, nur neuer runden anzahl

                          logic->new_Round_c(0); // runde incrementieren


                      EV<<"Slack= "<<logic->get_Slack()<<" | round= "<<logic->get_round()<<" | k= "<<logic->get_k()<<endl;
                      uint32_t new_local_threshold=logic->calc_local_threshold();




                      logic->set_local_treshold(new_local_threshold);
                       // TO DO
                       // Evenetuell noch hier einbauen, das er direkt einen der k tresholds incrementiert, wenn er hier den neuen threshold überschreiten sollte


                      //EV << "neuer Slack/threshld ist "<<logic->get_Slack()<< "\n";
                      EV << "neuer localer_threshold ist "<<new_local_threshold<< "\n";

                      if(distanceToCoordinator==0) EV << "Coordinator veranlässt eine neue runde an alle: "<<logic->get_round()<< "\n";
                      else                         EV << "jemand veranlässt eine neue runde an alle: "<<logic->get_round()<< "\n";



                                  send_new_threshold(new_local_threshold,8,0);

                                 if(new_local_threshold == 1)childs_local_threshold_reached_one=true;
                          } // end of logic->run_var==logic->get_k()
                      } // end of childs_local_threshold_reached_one
           }}}
       }
           break;
   case 8 : // 4 = "new_Round()" distribute new threshold or somethin (for round based algorithms) for Algo2
     {
         //EV<<"MY old Slack is "<<logic->get_Slack()<<endl;
         //EV<<"MY threshold is "<<logic->get_local_threshold()<<endl;
         EV<<"MY new Slack is "<<read_uint32_t(msg,4)<<endl;
         EV<<"MY local count is "<<logic->get_count()<<endl;
         EV<<"My local counter is"<<counter<<endl;

             ASSERT(read_uint32_t(msg,4)!=0);
             ASSERT(isSet); // you can just get an update if u are Set
             ASSERT(!par("initiator")); // the coordinator never gets a new_Round message !
             ASSERT(logic->get_local_threshold()!=0); // if threshold reached zero, there is somethin fishy
             //ASSERT(logic->get_local_threshold()!=1); // without  Multihop this ASSERT is TRUE
             if(logic->get_Slack()==0) ASSERT(logic->get_k()!=1); // Slack kann 0 sein, wenn dieser Node grade eben selbst den lokalen ast threshold erreicht hat durch andere (siehe 5)


             bool its_new_round=false;


             if(read_uint32_t(msg,8)==1){ // restart of an algo
             logic->reset_round(); EV<<"some algo upwards finished"<<endl; // oder sogar immer resetten
             childs_local_threshold_reached_one=false;its_new_round=true;
             }


             logic->reset_round();
             logic->run_var=0;


             EV<<"MY round is "<<logic->get_round()<<endl;

             logic->set_local_branch_threshold(read_uint32_t(msg,4));
             logic->set_local_Slack(read_uint32_t(msg,4));
             EV <<"b = "<<logic->get_local_branch_threshold()<<endl;




             // schau ob durch neues b vieleicht counter direkt erreicht ist
             if(does_this_node_finished_these_round()){
                   if(logic->get_local_branch_threshold()!=1)
                   ASSERT(does_this_node_finished_these_round()==false); // er kann maximal einmal überschritten sein
                  // else
                  // {while(does_this_node_finished_these_round()==true){}}
             }
             logic->set_local_Slack(logic->get_local_branch_threshold()-counter);
             counter=0;







             if(logic->get_local_threshold()!=1 || its_new_round){
            // Wenn threshold bereits eins ist tue nichts, außer wenn eine neue runde beginnt, weil alle unteren knoten werden dann auch schon local threshold 1 haben

             if(logic->new_Round(read_uint32_t(msg,4))){ // hat der local_count den local_threshold erreicht?
                 // An Update results in a new reached threshold
                 EV<<"MY new threshold is "<<logic->get_local_threshold()<<endl;

                 if(logic->get_k()==1){
                 EV << "sendet threshold_reached of "<<read_uint32_t(msg,4)<<" (due to new round)"<< "\n";

                             uint32_t                   count=2;
                             uint32_t local_threshold_reached=7;

                             if(read_uint32_t(msg,4)==1){
                                                                                       send_event(palId_id()+sendCounter * amount_of_clients,count);
                                while(logic->get_count()!=0){ debug_val++;logic->decrement_count();send_event(palId_id()+sendCounter * amount_of_clients,count);} // theorettisch optimierungspotenzial, die counts alle in eine nachricht zu packen, aber nö
                             }
                             else
                             send_event(palId_id()+sendCounter * amount_of_clients,local_threshold_reached,read_uint32_t(msg,4));
                 }
                 else{


                          logic->run_var++;
                          counter+=logic->get_local_threshold();

                          if(logic->run_var==logic->get_k()){
                              ASSERT(false); // dat ding kann hier wohl raus
                                    EV <<logic->run_var<<" times were this reached "<< logic->get_local_threshold() << "\n";
                                    logic->new_Round_c(0);
                                    logic->set_local_treshold(logic->calc_local_threshold());

                                    EV <<"count is "<<counter<<endl;
                                    logic->run_var=0;
                                }
                 }
             }

             EV << "runde: "<< logic->get_round()<< " | local_count: "<< logic->get_count() <<"\n";
             EV<<"  MY threshold is "<<logic->get_local_threshold()<<endl;
             EV<<"  MY branch threshold is "<<logic->get_local_branch_threshold()<<endl;


             if(logic->get_k()==1)ASSERT(logic->get_local_threshold()==logic->get_Slack());




              // Distribute new Slack/threshold further
             send_new_threshold((uint32_t)logic->get_local_threshold(),8,read_uint32_t(msg,8));

             } // end of scope






     }
             break;
   default  :  EV_ERROR <<"Error unknown message: "<< read_uint32_t(msg) <<endl; break;
   }
    delete msg;
} // end of handleIndication


bool node_handler::does_this_node_finished_these_round(){
    bool fin=false;
//    if (counter >= threshold) {printResult();node_handler::running = false;} // Optimierung !!! (Spart bei Algo1+Line eine Nachricht, (Letzte nachricht um genau zu sein))
//    else

            if(logic->isAlgo1()){
                if(counter>=logic->get_local_branch_threshold()){ fin=true;
                 // ASSERT(logic->get_Slack()==0); muss net unbedingt der fall sein, es kann sein, das beim erhalten eienes neuen schwellenwertes, der breits überschritten ist sieh nachricht 4
                //ASSERT(counter==logic->get_local_branch_threshold()); // (kann der Fall sein siehe meine aufzeichnung bsp 42)
                if(counter==1 && temp_count==0) {  send_event(palId_id()+sendCounter * amount_of_clients,2); counter-=1;}
                else     {
                    do{
                        send_event(palId_id()+sendCounter * amount_of_clients,5,logic->get_local_branch_threshold()+temp_count); temp_count=0;
                        counter-=logic->get_local_branch_threshold();
                    }while(counter>=logic->get_local_branch_threshold());
                }



                }
            }
            else { if(counter>=logic->get_Slack()){ fin=true;

                if(logic->get_local_branch_threshold()==1) {
                    {
                    send_event(palId_id()+sendCounter * amount_of_clients,2); counter-=1;
                    }//while(counter!=0);
                }
                else
                {
                send_event(palId_id()+sendCounter * amount_of_clients,7,logic->get_local_branch_threshold()); counter-=logic->get_Slack();
                while(counter>=logic->get_local_branch_threshold())
                {
                send_event(palId_id()+sendCounter * amount_of_clients,7,logic->get_local_branch_threshold()); counter-=logic->get_local_branch_threshold();
                };
                }


            if(logic->get_local_branch_threshold()!=1) // wenn local_branch_threshold 1 ist, wird im folgenden der ganze ast jeden count direkt reporten
            {
            logic->reset_round();
            logic->set_local_Slack(logic->get_local_branch_threshold());
            logic->set_local_treshold(logic->calc_local_threshold());
             EV <<"we reached "<<logic->get_local_branch_threshold()<<endl;
             childs_local_threshold_reached_one=false;
             send_new_threshold(logic->get_local_threshold(),8,1); // die nachricht müsste beim k-ten mal nicht gesendet werden, da denn eh ein neuer ini von oben kommt , aber ein node kennt nicht seine nachbarn !
            }
            }
            }



    return fin;
};


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
    EV << "SE_sending event"<<(int)status_code<< endl;
    EV << "SE_value "<<para<< endl;
    AirframePtr frame = make_checked<Airframe>();

    (*frame) << status_code; // default status_code = 2

    uint32_t sequence =  s+amount_of_clients ; // sequencenumber is importent in case an acknowledgement get lost
    (sequence >= (UINT32_MAX-amount_of_clients)) ? (EV_WARN<<"SE_sequence_number_warap_around"<<endl) : (EV<<"SE_sequence_number_are_ok "<<sequence<<endl);
    (*frame) << sequence;


    if(status_code==5 || status_code==7) {(*frame) << para;messages_thresholdreached++;}
    else {messages_sendCount++;}


    sendRequest(new DataRequest(out, frame, createCallback(&node_handler::resp)));
    sendCounter++;
    node_handler::messagesSend++;
    EV << "SE_Send_Event: Messages " <<node_handler::messagesSend<< endl;
    WATCH(sendCounter);
    WATCH(node_handler::messagesSend);
}

void node_handler::send_new_threshold(uint32_t new_local_threshold,uint32_t status_code,uint32_t restart){

    int size=(int) subs.getSize();

EV<<"jetzt wird eine neue runde verschickt"<<endl;
    if(last_threshold_send!=new_local_threshold)
    { EV<<" wird auch :)"<<endl;



               for(int i=0;i!=(size);i++)
               //for(int i=(size-1);i!=(-1);i--)
               {
                 AirframePtr frame = make_checked<Airframe>();
                           (*frame) << status_code;
                           (*frame) << new_local_threshold;

                 if(status_code==4) {sendCounter++;node_handler::messagesSend++;messages_newRound++;
                                     EV << "neue runde an "<<subs[i]<<" (messages: "<<node_handler::messagesSend<< ")\n";}
                 if(status_code==8) {sendCounter++;node_handler::messagesSend++;messages_newRound++;
                                                      EV << "neue runde an "<<subs[i]<<" (messages: "<<node_handler::messagesSend<< ")\n";
                                     (*frame) << restart;      }
                 if(status_code==6)  EV << "sending start observing further to "<<subs[i]<< endl;

                 sendRequest(new DataRequest(subs[i], frame,createCallback(&node_handler::resp)));

               }

               last_threshold_send=new_local_threshold;
    }
}


void node_handler::printResult(){
    char buf[50]; // 50
    sprintf(buf, "threshold reached with %d messages send overall",node_handler::messagesSend);
    getDisplayString().setTagArg("t", 0, buf);
    bubble(buf);
    std::cout << "counter = "<< counter  << endl;
    std::cout << "threshhold reached with: "<< node_handler::messagesSend << " messages over all "<<"\n" << endl;
    std::cout << "SendCount="<<messages_sendCount<<" | threshold_reached="<<messages_thresholdreached<<" | new_Round="<<messages_newRound<<endl;
    std::cout << "Debug output: einzelnde gesendete counts="<<debug_val<<endl;

   std::ofstream file;
   file.open("results//results", std::fstream::app);
   file << this->getSimulation()->getActiveEnvir()->getConfigEx()->getVariable("runid")<<" | counter="<<counter<<" | messages="<<node_handler::messagesSend<<endl;
   file.close();

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
