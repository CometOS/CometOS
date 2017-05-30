

#include "Algo1.h"


Algo1::Algo1(unsigned long int Slack,unsigned long int k):Algo(Slack,k){}

void Algo1::local_threshold_reached(){



}


bool Algo1::new_Round(){
    this->round++;

    Slack-=this->get_local_threshold();


    double d1=static_cast<double>(Slack);
    double d2=static_cast<double>(k);
    double d3=ceil(d1/d2);

    unsigned int new_local_threshold=static_cast<unsigned long int>(d3);
    if(new_local_threshold<=1){
       new_local_threshold =1;
       }

    this->local_threshold = new_local_threshold;





    if(this->local_count >= new_local_threshold){
             local_count -= new_local_threshold;
             return true;
    }

//    if(this->local_count >= new_local_threshold && new_local_threshold!=1){
//               return true;
//    }


    return false;
}
