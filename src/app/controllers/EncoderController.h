
#pragma once
#include <Arduino.h>
#include "../../config/PinMap.h"
#include "../../config/Defaults.h"

struct EncoderEvents {
    int delta = 0;
    bool shortPress = false;
    bool longPress = false;
    bool veryLongPress = false;
};

class EncoderController {
public:
    void begin(const EncoderConfig& cfg_) {
        cfg = cfg_;
        pinMode(PIN_ENC_A, INPUT_PULLUP);
        pinMode(PIN_ENC_B, INPUT_PULLUP);
        pinMode(PIN_ENC_BTN, INPUT_PULLUP);
        lastA = digitalRead(PIN_ENC_A);
        btnStable = digitalRead(PIN_ENC_BTN);
        btnLastChangeMs = millis();
    }

    EncoderEvents poll() {
        EncoderEvents e;
        int a = digitalRead(PIN_ENC_A);
        if (a != lastA) {
            lastA = a;
            if (digitalRead(PIN_ENC_B) != a) e.delta = 1;
            else e.delta = -1;
        }

        uint32_t now = millis();
        int raw = digitalRead(PIN_ENC_BTN);
        if (raw != btnStable) {
            if (now - btnLastChangeMs >= cfg.btnDebounceMs) {
                btnStable = raw;
                btnLastChangeMs = now;
                if (btnStable == LOW) {
                    pressStartMs = now;
                    pressed = true;
                } else {
                    if (pressed) {
                        uint32_t dur = now - pressStartMs;
                        if (dur >= cfg.veryLongPressMs) e.veryLongPress = true;
                        else if (dur >= cfg.longPressMs) e.longPress = true;
                        else e.shortPress = true;
                    }
                    pressed = false;
                }
            }
        } else {
            btnLastChangeMs = now;
        }
        return e;
    }

    // NOTE: UI owns meaning of events; encoder controller only reports events.

private:
    EncoderConfig cfg;
    int lastA = HIGH;

    int btnStable = HIGH;
    uint32_t btnLastChangeMs = 0;
    uint32_t pressStartMs = 0;
    bool pressed = false;

    // (legacy) kept for backward compatibility; not used by the new UI.
    uint8_t legacyMode = 0;
};
