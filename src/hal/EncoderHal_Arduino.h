#pragma once
#include <Arduino.h>
#include "EncoderHal.h"
#include "../config/PinMap.h"

class EncoderHal_Arduino : public EncoderHal {
public:
    void beginPins();

    uint32_t millisNow() const override { return ::millis(); }

    int readA() const override { return ::digitalRead(PIN_ENC_A); }
    int readB() const override { return ::digitalRead(PIN_ENC_B); }
    int readBtn() const override { return ::digitalRead(PIN_ENC_BTN); }

    void attachABInterrupts(void (*isr)()) override;
    void detachABInterrupts() override;

    void enterCritical() override { noInterrupts(); }
    void exitCritical() override { interrupts(); }
};