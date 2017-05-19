


#ifndef ALGO_H_
#define ALGO_H_



class Algo{

public:

        Algo();
        Algo(unsigned long int local_threshold,unsigned long int k);
virtual~Algo();
    
    
    virtual void local_threshold_reached(){};
    virtual void new_Round(){};

    
    
protected:

    unsigned long int local_count;
    unsigned long int local_threshold;
    unsigned long int k; // number of Observer behind this node

};

#endif /* ALGO_H_ */









