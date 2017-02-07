#include "PureSniffer.h"
#include "CsmaLayer.h"
#include "MacSymbolCounter.h"

using namespace cometos;


PureSniffer& PureSniffer::getInstance() {
    static PureSniffer pureSniffer;
    return pureSniffer;
}

void timerInterrupt() {
}

void PureSniffer::init(cometos::Callback<void(uint8_t* data, uint8_t length, uint32_t sfdTimestamp)> newCallback) {
    this->callback = newCallback;
    auto rf = Rf231::getInstance();
    mac_setRadioDevice(rf);

    mac_result_t result = RFA1Driver_init(1234,0,MAC_DEFAULT_CHANNEL,0x00,&ackCfg,&backoffCfg);
    ASSERT(result == MAC_SUCCESS);

    // Enable MacSymbolCounter
    MacSymbolCounter::getInstance().init(CALLBACK_FUN(timerInterrupt));
	uint8_t trx_ctrl_1 = rf->readRegister(AT86RF231_REG_TRX_CTRL_1);
	trx_ctrl_1 |= AT86RF231_TRX_CTRL_1_MASK_IRQ_2_EXT_EN;
	rf->writeRegister(AT86RF231_REG_TRX_CTRL_1, trx_ctrl_1);
}

void PureSniffer::receive(message_t* msg) {
    // -2 since the AT86RF231 captures at the end of the PHR (+6us) instead at the end of the SFD as the ATmega256RFR2
    uint32_t sfdTimestamp = MacSymbolCounter::getInstance().getCapture() - 2;

    PureSniffer::getInstance().callback(msg->data, msg->phyPayloadLen + 2, sfdTimestamp);
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
