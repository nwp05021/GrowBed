#include "EncoderHal_Arduino.h"

void EncoderHal_Arduino::beginPins() {
    pinMode(PIN_ENC_A, INPUT_PULLUP);
    pinMode(PIN_ENC_B, INPUT_PULLUP);
    pinMode(PIN_ENC_BTN, INPUT_PULLUP);
}

void EncoderHal_Arduino::attachABInterrupts(void (*isr)()) {
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_A), isr, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_B), isr, CHANGE);
}

void EncoderHal_Arduino::detachABInterrupts() {
    detachInterrupt(digitalPinToInterrupt(PIN_ENC_A));
    detachInterrupt(digitalPinToInterrupt(PIN_ENC_B));
}