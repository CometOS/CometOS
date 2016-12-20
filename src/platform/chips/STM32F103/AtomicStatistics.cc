#include "AtomicStatistics.h"
#include "palExec.h"

using namespace cometos;

bool AtomicStatistics::logInAtomic = false;
bool AtomicStatistics::logInIRQ = false;
uint8_t AtomicStatistics::logLast = 0;
uint16_t AtomicStatistics::logLastAtomicBegin;
uint16_t AtomicStatistics::logDuration[LOGDURATIONLENGTH];

void AtomicStatistics::print() {
    uint8_t logLastTmp;
    __disable_irq();
    logLastTmp = logLast;
    __enable_irq();

    cometos::getCout() << "AT ";
    for(int i = LOGDURATIONLENGTH-1; i >= 0; i--) {
        uint16_t value;
        __disable_irq();
        value = logDuration[(logLastTmp+i)%LOGDURATIONLENGTH];
        __enable_irq();

        cometos::getCout() << value << " ";
    }
    cometos::getCout() << cometos::endl;
}
