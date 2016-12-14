#ifndef PURE_SNIFFER_H
#define PURE_SNIFFER_H

#include "rf231.h"
#include "RFA1DriverLayer.h"
#include "mac_interface_ext.h"
#include "OutputStream.h"

namespace cometos {

class PureSniffer {
public:
    static PureSniffer& getInstance();

    void init(cometos::Callback<void(uint8_t* data, uint8_t length)> callback);
    void receive(message_t* msg);

private:
    PureSniffer() {}
    PureSniffer(PureSniffer&) = delete;
    PureSniffer& operator=(const PureSniffer&) = delete;

    cometos::Callback<void(uint8_t* data, uint8_t length)> callback;

    mac_ackCfg_t ackCfg;
    mac_backoffCfg_t backoffCfg;
};

}

#endif
