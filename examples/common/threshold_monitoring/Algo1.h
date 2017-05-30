

#include "Algo.h"


#ifndef ALGO1_H_
#define ALGO1_H_

class Algo1 : public Algo{

public:
     Algo1(){};
     Algo1(unsigned long int local_threshold,unsigned long int k);
    ~Algo1(){};


    void local_threshold_reached();
    bool new_Round();

};

#endif /* ALGO_H_ */
