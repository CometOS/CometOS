


#ifndef ALGO_H_
#define ALGO_H_

#include <cmath>


class Algo{

public:

        Algo();
        Algo(unsigned long int local_threshold,unsigned long int k);
virtual~Algo();
    


    
    virtual void local_threshold_reached(){};
    virtual bool new_Round(unsigned long int new_local_threshold){}; // for clients only
            void new_Round_c(unsigned long int reduction); // for Coordinator only

    unsigned long int calc_local_threshold();

   void set_local_treshold(unsigned long int);
   void set_local_Slack(unsigned long int);
   void set_k(unsigned long int);


            bool count();

unsigned long int get_local_threshold(){return local_threshold_int;}
unsigned long int get_count()          {return local_count        ;}
unsigned long int get_round()          {return round              ;}
unsigned long int get_Slack()          {return Slack              ;}
unsigned long int get_k()              {return k                  ;}

    
protected:

    volatile unsigned long int local_count;
             unsigned long int local_threshold_int;

             unsigned long int round;
             unsigned long int Slack;
             unsigned long int k;

};

#endif /* ALGO_H_ */









