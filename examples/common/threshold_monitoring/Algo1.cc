

#include "Algo1.h"





namespace cometos {

unsigned long int Algo1::calc_local_threshold(){
    double  d1=static_cast<double>(this->Slack);
    double  d2=static_cast<double>(this->k);
    return static_cast<unsigned long int>(ceil(d1/d2));
}



bool Algo1::new_Round(unsigned long int new_local_Slack){
    this->round++;

    //set_local_Slack(new_local_Slack); wurde jetzt in funktion verschoben
    set_local_treshold(calc_local_threshold());



    if(this->local_count >= local_threshold_int){
             local_count -= local_threshold_int;
             return true;
    }

    return false;
}


void Algo1::handle_message(DataIndication* msg,void * t){

uint32_t rf=palId_id();

//((void)0, !((omnetpp::LOGLEVEL_INFO >= omnetpp::LOGLEVEL_TRACE) && \
//    omnetpp::cLog::runtimeLogPredicate(t.getThisPtr(), omnetpp::LOGLEVEL_INFO, nullptr))) ? \
//    omnetpp::cLogProxy::dummyStream : omnetpp::cLogProxy(getThisPtr(), omnetpp::LOGLEVEL_INFO, nullptr, "/home/cfu3434/cometos/examples/common/threshold_monitoring/node_handler.cc", 117, __FUNCTION__).getStream()<<"dd"<<endl;

 //   EV <<"node "<< palId_id()<<" is generating one event"<< endl;

}

}
