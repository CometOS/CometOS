



#include "Algo.h"

Algo::Algo(unsigned long int Slack,unsigned long int k):Slack(Slack),k(k),local_count(0),round(0),threshold_reached_one(false){
    double d1=static_cast<double>(Slack);
    double d2=static_cast<double>(k);
    this->local_threshold = d1/d2;
};

Algo::~Algo(){}

void Algo::initialize(unsigned long int k,unsigned long int local_threshold){
this->k=k;
this->local_threshold_int=local_threshold;
}


bool Algo::count(){

    local_count++;
    if(local_count>=local_threshold){
       local_count-=static_cast<unsigned long int>(ceil(local_threshold));
        return true;
    }
    return false;
}

