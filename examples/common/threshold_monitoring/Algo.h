


#ifndef ALGO_H_
#define ALGO_H_

#include <cmath>


class Algo{

public:

        Algo();
        Algo(unsigned long int local_threshold,unsigned long int k);
virtual~Algo();
    

            void initialize(unsigned long int k,unsigned long int S);
    
    virtual void local_threshold_reached(){};
    virtual bool new_Round(){};


            bool count();

         int get_local_threshold(){
             double t=ceil(local_threshold);
             return (int)t;}
         int get_count(){return local_count;}
         int get_round(){return round;}
         int get_Slack(){return Slack;}
         int get_k(){return k;}
         bool get_threshold_reached_one(){return threshold_reached_one;}
    
protected:

    volatile unsigned long int local_count;                // Nur die
    volatile double            local_threshold;
             unsigned long int local_threshold_int;        // beiden am ende

    unsigned long int round;
    unsigned long int Slack;
    unsigned long int k; // number of Observer behind this node

    bool threshold_reached_one;




};

#endif /* ALGO_H_ */









