

#include "Algo.h"


#ifndef ALGO1_H_
#define ALGO1_H_

namespace cometos {

class Algo1 : public Algo{

public:

     Algo1():Algo(){}
    ~Algo1(){};

    unsigned long int calc_local_threshold();
    bool new_Round(unsigned long int);
    bool isAlgo1(){return true;};

    void handle_message(DataIndication* msg,void *);

};

}

#endif /* ALGO_H_ */
