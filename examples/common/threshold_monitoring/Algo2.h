

#include "Algo.h"


#ifndef ALGO2_H_
#define ALGO2_H_

namespace cometos {

class Algo2 : public Algo{

public:

     Algo2():Algo(){}
    ~Algo2(){};

    unsigned long int calc_local_threshold();
    bool new_Round(unsigned long int);
    bool isAlgo1(){return false;};

};

}

#endif /* ALGO_H_ */
