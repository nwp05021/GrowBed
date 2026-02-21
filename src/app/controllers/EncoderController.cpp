#include "EncoderController.h"

EncoderController* EncoderController::instance = nullptr;

EncoderController::EncoderController(EncoderHal& hal_)
    : hal(hal_) {}

void EncoderController::begin(const EncoderConfig& cfg_) {
    instance = this;

    uint8_t initialAB =
        (hal.readA() << 1) | hal.readB();

    int initialBtn = hal.readBtn();

    logic.begin(cfg_, initialAB, initialBtn);

    hal.attachABInterrupts(isrRouter);
}

EncoderEvents EncoderController::poll() {

    hal.enterCritical();
    logic.takeIsrDeltaSnapshot();
    hal.exitCritical();

    uint32_t now = hal.millisNow();
    int btnRaw = hal.readBtn();

    return logic.poll(now, btnRaw);
}

void EncoderController::isrRouter() {
    if (instance)
        instance->handleIsr();
}

void EncoderController::handleIsr() {
    uint8_t ab = (hal.readA() << 1) | hal.readB();
    logic.onIsrAB(ab);
}