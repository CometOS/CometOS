

#include "Algo2.h"

namespace cometos {

unsigned long int Algo2::calc_local_threshold(){
    double  d1=static_cast<double>(this->Slack);
    double  d2=static_cast<double>(this->k);
    double  d3=static_cast<double>((this->round+1));
    double  d4=floor(d1/(d2*pow(2,d3)));
    unsigned int long i4=static_cast<unsigned long int>(d4);
    if(i4==0) return 1; // threshold 0 macht kein sinn !
    else return i4;
}

bool Algo2::new_Round(unsigned long int new_local_Slack){




    if(this->get_k()==1) // wenn er keine kinder nodes hat, bekommt er seinen threshold
    set_local_treshold(new_local_Slack);
    else
    set_local_treshold(calc_local_threshold());

  //  this->round++;
  // neue runde wird nur gestartet wenn k thresholds erreicht wurden


    if(this->local_count >= local_threshold_int){
             local_count -= local_threshold_int;
             return true;
    }

    return false;
}


}
