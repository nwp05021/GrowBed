#pragma once
#include <Arduino.h>
#include "../../config/PinMap.h"
#include "../../config/Defaults.h"

// EncoderEvents are consumed by UiController.
// - delta: -1/0/+1 (one detent = one tick)
// - shortPress: released before longPress threshold
// - longPress: fired on RELEASE if held >= longPressMs AND veryLong not triggered
// - veryLongPress: fired once when held >= veryLongPressMs (while still pressed)
struct EncoderEvents {
    int delta = 0;
    bool shortPress = false;
    bool longPress = false;
    bool veryLongPress = false;
};

class EncoderController {
public:
    EncoderController() = default;

    void begin(const EncoderConfig& cfg_) {
        cfg = cfg_;

        pinMode(PIN_ENC_A, INPUT_PULLUP);
        pinMode(PIN_ENC_B, INPUT_PULLUP);
        pinMode(PIN_ENC_BTN, INPUT_PULLUP);

        // init rotation state
        instance = this;
        prevAB = readAB();
        isrAcc = 0;
        isrDelta = 0;

        // attach interrupts (A/B)
        attachInterrupt(digitalPinToInterrupt(PIN_ENC_A), isrRouter, CHANGE);
        attachInterrupt(digitalPinToInterrupt(PIN_ENC_B), isrRouter, CHANGE);

        // button debounce init
        const uint32_t now = millis();
        btnStable = digitalRead(PIN_ENC_BTN);
        btnRawPrev = btnStable;
        btnRawChangeMs = now;
        pressed = (btnStable == LOW);
        pressStartMs = pressed ? now : 0;
        veryLongFired = false;
    }

    // Call frequently from loop()
    EncoderEvents poll() {
        EncoderEvents e;
        const uint32_t now = millis();

        // ---- Rotation: ISR-decoded gray code (NO misses) ----
        // We emit at most one tick per poll, keeping the remainder.
        int16_t d = 0;
        noInterrupts();
        d = isrDelta;
        if (d > 0) isrDelta -= 1;
        else if (d < 0) isrDelta += 1;
        interrupts();

        if (d > 0) e.delta = +1;        // right = +1
        else if (d < 0) e.delta = -1;   // left  = -1

        // ---- Button: debounced + state machine ----
        // Policy:
        // - VeryLong fires while holding (Engineering).
        // - Long/Short fire on RELEASE, but only if VeryLong did NOT fire.
        const int raw = digitalRead(PIN_ENC_BTN);

        // track raw changes
        if (raw != btnRawPrev) {
            btnRawPrev = raw;
            btnRawChangeMs = now;
        }

        // update stable state after debounce interval
        if (raw != btnStable && (now - btnRawChangeMs) >= cfg.btnDebounceMs) {
            btnStable = raw;

            if (btnStable == LOW) {
                // pressed
                pressed = true;
                pressStartMs = now;
                veryLongFired = false;
            } else {
                // released
                if (pressed) {
                    const uint32_t held = now - pressStartMs;

                    if (!veryLongFired) {
                        if (held >= cfg.longPressMs) e.longPress = true;
                        else e.shortPress = true;
                    }
                }
                pressed = false;
            }
        }

        // fire very-long while holding (Engineering shortcut)
        if (pressed && !veryLongFired) {
            const uint32_t held = now - pressStartMs;
            if (held >= cfg.veryLongPressMs) {
                e.veryLongPress = true;
                veryLongFired = true;
            }
        }

        return e;
    }

private:
    EncoderConfig cfg;

    // ---- Rotation (ISR) ----
    volatile uint8_t prevAB = 0;
    volatile int8_t  isrAcc = 0;
    volatile int16_t isrDelta = 0;

    static EncoderController* instance;

    static void isrRouter() {
        if (instance) instance->handleIsr();
    }

    inline void handleIsr() {
        // Transition table for quadrature decoding
        static const int8_t TBL[16] = {
             0, -1, +1,  0,
            +1,  0,  0, -1,
            -1,  0,  0, +1,
             0, +1, -1,  0
        };

        const uint8_t curr = readAB();
        if (curr == prevAB) return;

        const uint8_t idx = (uint8_t)((prevAB << 2) | curr);
        const int8_t step = TBL[idx];

        if (step != 0) {
            isrAcc += step;

            // emit one detent per +/-4, keep remainder (prevents loss when spinning fast)
            if (isrAcc >= 4) {
                isrDelta += 1;   // right = +1
                isrAcc -= 4;
            } else if (isrAcc <= -4) {
                isrDelta -= 1;   // left = -1
                isrAcc += 4;
            }
        }

        prevAB = curr;
    }

    // ---- Button debounce + state ----
    int btnStable = HIGH;
    int btnRawPrev = HIGH;
    uint32_t btnRawChangeMs = 0;

    bool pressed = false;
    uint32_t pressStartMs = 0;
    bool veryLongFired = false;

    inline uint8_t readAB() const {
        // bit1=A, bit0=B
        return (uint8_t)((digitalRead(PIN_ENC_A) << 1) | digitalRead(PIN_ENC_B));
    }
};

inline EncoderController* EncoderController::instance = nullptr;
