


#ifndef ALGO_H_
#define ALGO_H_

#include <cmath>


#include "Endpoint.h" // for DataIndication
#include "logging.h"
#include "Airframe.h"






namespace cometos {

class Algo{

public:

        Algo();
        Algo(unsigned long int local_threshold,unsigned long int k);
virtual~Algo();
    


    virtual unsigned long int calc_local_threshold(){};
    virtual bool isAlgo1(){};
    virtual bool new_Round(unsigned long int){}; // for clients only
            void new_Round_c(unsigned long int reduction); // for Coordinator only



            virtual void handle_message(DataIndication* msg,void *){};



   void set_local_treshold(unsigned long int);
   void set_local_Slack(unsigned long int);
   void set_k(unsigned long int);
   void set_local_branch_threshold(unsigned long int);
   void reset_round(){round=0;};
   void decrement_count(){local_count--;};




            bool count();


unsigned long int get_local_threshold()       {return local_threshold_int   ;}
unsigned long int get_count()                 {return local_count           ;}
unsigned long int get_round()                 {return round                 ;}
unsigned long int get_Slack()                 {return Slack                 ;}
unsigned long int get_k()                     {return k                     ;}
unsigned long int get_local_branch_threshold(){return local_branch_threshold;}


       unsigned int run_var; // only for Algo2

    
protected:

    volatile unsigned long int local_count;
             unsigned long int local_threshold_int;
             unsigned long int local_branch_threshold;


             unsigned long int round;
             unsigned long int Slack;
             unsigned long int k;

};

}

#endif /* ALGO_H_ */









