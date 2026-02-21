#include <Arduino.h>
#include "EncoderLogic.h"

void EncoderLogic::begin(const EncoderConfig& cfg_,
                         uint8_t initialAB,
                         int initialBtnRaw) {
    cfg = cfg_;

    prevAB = initialAB;
    acc = 0;
    isrDeltaAccum = 0;
    pendingDelta = 0;

    btnStable = initialBtnRaw;
    btnRawPrev = btnStable;
    btnRawChangeMs = 0;

    pressed = (btnStable == LOW);
    pressStartMs = 0;
    veryLongFired = false;
}

void EncoderLogic::onIsrAB(uint8_t currAB) {
    static const int8_t TBL[16] = {
         0, -1, +1,  0,
        +1,  0,  0, -1,
        -1,  0,  0, +1,
         0, +1, -1,  0
    };

    if (currAB == prevAB) return;

    const uint8_t idx = (uint8_t)((prevAB << 2) | currAB);
    const int8_t step = TBL[idx];

    if (step != 0) {
        acc += step;

        if (acc >= 4) {
            isrDeltaAccum += 1;
            acc -= 4;
        } else if (acc <= -4) {
            isrDeltaAccum -= 1;
            acc += 4;
        }
    }

    prevAB = currAB;
}

int16_t EncoderLogic::takeIsrDeltaSnapshot() {
    const int16_t snap = isrDeltaAccum;
    isrDeltaAccum = 0;
    pendingDelta += snap;
    return snap;
}

EncoderEvents EncoderLogic::poll(uint32_t nowMs, int btnRaw) {
    EncoderEvents e;

    // ----- rotation -----
    if (pendingDelta > 0) {
        e.delta = +1;
        pendingDelta -= 1;
    }
    else if (pendingDelta < 0) {
        e.delta = -1;
        pendingDelta += 1;
    }

    // ----- button debounce -----
    if (btnRaw != btnRawPrev) {
        btnRawPrev = btnRaw;
        btnRawChangeMs = nowMs;
    }

    if (btnRaw != btnStable &&
        (nowMs - btnRawChangeMs) >= cfg.btnDebounceMs) {

        btnStable = btnRaw;

        if (btnStable == LOW) {
            pressed = true;
            pressStartMs = nowMs;
            veryLongFired = false;
        }
        else {
            if (pressed) {
                uint32_t held = nowMs - pressStartMs;

                if (!veryLongFired) {
                    if (held >= cfg.longPressMs)
                        e.longPress = true;
                    else
                        e.shortPress = true;
                }
            }
            pressed = false;
        }
    }

    if (pressed && !veryLongFired) {
        uint32_t held = nowMs - pressStartMs;

        if (held >= cfg.veryLongPressMs) {
            e.veryLongPress = true;
            veryLongFired = true;
        }
    }

    return e;
}