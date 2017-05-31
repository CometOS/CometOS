

#include "Algo1.h"




void Algo1::local_threshold_reached(){



}


bool Algo1::new_Round(unsigned long int new_local_Slack){
    this->round++;

    set_local_Slack(new_local_Slack);
    set_local_treshold(calc_local_threshold());



    if(this->local_count >= local_threshold_int){
             local_count -= local_threshold_int;
             return true;
    }

    return false;
}
