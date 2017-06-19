



#include "Algo.h"

namespace cometos {

Algo::Algo():local_count(0),round(0),run_var(0){

};

Algo::~Algo(){}





void Algo::set_local_treshold(unsigned long int t){local_threshold_int = t;}
void Algo::set_local_Slack(unsigned long int S){Slack=S;}
void Algo::set_k(unsigned long int k){this->k=k;}
void Algo::set_local_branch_threshold(unsigned long int local_branch_threshold){this->local_branch_threshold=local_branch_threshold;}




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



}
