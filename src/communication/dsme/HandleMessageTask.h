#ifndef HANDLEMESSAGETASK_H
#define HANDLEMESSAGETASK_H

namespace dsme {

typedef Delegate<void(IDSMEMessage* msg)> receive_delegate_t;

class HandleMessageTask : public cometos::Task {
public:
    HandleMessageTask() :
            message(nullptr), occupied(false) {
    }

    bool occupy(IDSMEMessage* message, receive_delegate_t receiveDelegate) {
        palExec_atomicBegin();
        if(this->occupied) {
            palExec_atomicEnd();
            return false;
        }
        else {
            this->occupied = true;
            this->message = message;
            this->receiveDelegate = receiveDelegate;
            palExec_atomicEnd();
            return true;
        }
    }

    bool isOccupied() {
        return occupied;
    }

    virtual void invoke() {
        IDSMEMessage* copy = nullptr;
        palExec_atomicBegin();
        {
            copy = message;
        }
        palExec_atomicEnd();

        receiveDelegate(copy);

        palExec_atomicBegin();
        {
            message = nullptr;
            occupied = false;
        }
        palExec_atomicEnd();
    }
private:
    IDSMEMessage* message;
    receive_delegate_t receiveDelegate;
    bool occupied;
};
}

#endif
