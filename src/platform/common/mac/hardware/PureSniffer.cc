#include "PureSniffer.h"
#include "CsmaLayer.h"

using namespace cometos;


PureSniffer& PureSniffer::getInstance() {
    static PureSniffer pureSniffer;
    return pureSniffer;
}

void PureSniffer::init(cometos::Callback<void(uint8_t* data, uint8_t length)> newCallback) {
    this->callback = newCallback;
    auto rf = Rf231::getInstance();
    mac_setRadioDevice(rf);

    mac_result_t result = RFA1Driver_init(1234,0,MAC_DEFAULT_CHANNEL,0x00,&ackCfg,&backoffCfg);
    ASSERT(result == MAC_SUCCESS);
}

void PureSniffer::receive(message_t* msg) {
    PureSniffer::getInstance().callback(msg->data, msg->phyPayloadLen + 2);
    // +2 : The RFA1DriverLayer signals the length without the FCS, but it is still in the data!
}

message_t* radioReceive_receive(message_t* msg) {
    PureSniffer::getInstance().receive(msg);
    return msg;
}

void csmaRadioAlarm_fired() {
    /* Do nothing. */
}

void salRadioAlarm_fired() {
    /* Do nothing. */
}

void tasklet_messageBufferLayer_run() {
    /* Do nothing. */
}

void radioSend_sendDone(message_t*, unsigned char) {
    /* Do nothing. */
}

void tasklet_radioAlarm_run() {
    /* Do nothing. */
}
