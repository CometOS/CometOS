



#include "Algo.h"

Algo::Algo():local_count(0),round(0){

};

Algo::~Algo(){}



unsigned long int Algo::calc_local_threshold(){
    double  d1=static_cast<double>(this->Slack);
    double  d2=static_cast<double>(this->k);
    return static_cast<unsigned long int>(ceil(d1/d2));
}

void Algo::set_local_treshold(unsigned long int t){local_threshold_int = t;}
void Algo::set_local_Slack(unsigned long int S){Slack=S;}
void Algo::set_k(unsigned long int k){this->k=k;}



void Algo::new_Round_c(unsigned long int reduction){
    this->round++;
    Slack-=reduction;
}


bool Algo::count(){

    local_count++;
    if(local_count>=local_threshold_int){
       local_count-=local_threshold_int;
        return true;
    }
    return false;
}
