#ifndef HANDLEMESSAGETASK_H
#define HANDLEMESSAGETASK_H

namespace dsme {

typedef Delegate<void(DSMEMessage* msg)> receive_delegate_t;

class HandleMessageTask : public cometos::Task {
public:
    HandleMessageTask() :
            message(nullptr) {
    }

    void set(DSMEMessage* message, receive_delegate_t receiveDelegate) {
        palExec_atomicBegin();
        {
            this->message = message;
            this->receiveDelegate = receiveDelegate;
        }
        palExec_atomicEnd();
        return;
    }

    virtual void invoke() {
        DSMEMessage* copy = nullptr;
        palExec_atomicBegin();
        {
            copy = message;
        }
        palExec_atomicEnd();

        receiveDelegate(copy);

        palExec_atomicBegin();
        {
            message = nullptr;
        }
        palExec_atomicEnd();
    }
private:
    DSMEMessage* message;
    receive_delegate_t receiveDelegate;
};
}

#endif
