#pragma once
#include <stdint.h>
#include "EncoderEvents.h"
#include "../../config/Defaults.h"

class EncoderLogic {
public:
    void begin(const EncoderConfig& cfg_,
               uint8_t initialAB,
               int initialBtnRaw);

    // ISR에서 호출 (아주 가볍게)
    void onIsrAB(uint8_t currAB);

    // Controller가 임계구역에서 호출
    int16_t takeIsrDeltaSnapshot();

    // loop에서 호출
    EncoderEvents poll(uint32_t nowMs, int btnRaw);

private:
    EncoderConfig cfg;

    // rotation decode
    uint8_t prevAB = 0;
    int8_t  acc = 0;

    volatile int16_t isrDeltaAccum = 0;
    int16_t pendingDelta = 0;

    // button state
    int btnStable = 1;
    int btnRawPrev = 1;
    uint32_t btnRawChangeMs = 0;

    bool pressed = false;
    uint32_t pressStartMs = 0;
    bool veryLongFired = false;
};