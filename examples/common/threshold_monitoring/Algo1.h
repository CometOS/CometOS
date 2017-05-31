

#include "Algo.h"


#ifndef ALGO1_H_
#define ALGO1_H_

class Algo1 : public Algo{

public:

     Algo1():Algo(){}

    ~Algo1(){};


    void local_threshold_reached();
    bool new_Round(unsigned long int);

};

#endif /* ALGO_H_ */
