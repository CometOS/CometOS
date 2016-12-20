#ifndef ATOMIC_STATISTICS_H
#define ATOMIC_STATISTICS_H

#include "OutputStream.h"
#include "MacSymbolCounter.h"
#include "cmsis_device.h"
#define LOGDURATIONLENGTH 50

namespace cometos {

class AtomicStatistics {
public:
    enum ID {
        ATOMIC_BLOCK = -1
    };

    static void begin(int8_t id) {
        if(id == ATOMIC_BLOCK) {
            if(logInIRQ) {
                return;
            }

            ASSERT(!logInAtomic);
            logInAtomic = true;
        }
        else {
            ASSERT(!logInIRQ);
            logInIRQ = true;
        }

        logLastAtomicBegin = TIM3->CNT;
    }

    static void end(int8_t id) {
        if(id == ATOMIC_BLOCK) {
            if(logInIRQ) {
                return;
            }
        }

        uint16_t diff = TIM3->CNT - logLastAtomicBegin;
        if(diff > 3) {
            ++logLast;
            logDuration[logLast%LOGDURATIONLENGTH] = diff;
        }

        if(id == ATOMIC_BLOCK) {
            logInAtomic = false;
        }
        else {
            logInIRQ = false;
        }
    }

    static void print();
private:
    static bool logInAtomic;
    static bool logInIRQ;
    static uint8_t logLast;
    static uint16_t logLastAtomicBegin;
    static uint16_t logDuration[LOGDURATIONLENGTH];
};
}

#endif
