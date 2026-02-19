
#pragma once
#include <Arduino.h>
#include "../config/PinMap.h"

class StepperHal_Drv8825 {
public:
    void begin() {
        pinMode(PIN_STEP, OUTPUT);
        pinMode(PIN_DIR, OUTPUT);
        pinMode(PIN_ENABLE, OUTPUT);
        if (PIN_MS1 != 255) pinMode(PIN_MS1, OUTPUT);
        if (PIN_MS2 != 255) pinMode(PIN_MS2, OUTPUT);
        if (PIN_MS3 != 255) pinMode(PIN_MS3, OUTPUT);
        enable(true);
    }

    void enable(bool on) {
        // DRV8825: ENABLE LOW = enabled
        digitalWrite(PIN_ENABLE, on ? LOW : HIGH);
        enabled = on;
    }

    bool isEnabled() const { return enabled; }

    void setDir(bool forward) {
        digitalWrite(PIN_DIR, forward ? HIGH : LOW);
    }

    void setMicrostepPins(bool ms1, bool ms2, bool ms3) {
        if (PIN_MS1 != 255) digitalWrite(PIN_MS1, ms1 ? HIGH : LOW);
        if (PIN_MS2 != 255) digitalWrite(PIN_MS2, ms2 ? HIGH : LOW);
        if (PIN_MS3 != 255) digitalWrite(PIN_MS3, ms3 ? HIGH : LOW);
    }

    inline void stepPulse() {
        // STEP minimum high pulse width: > 1.9us (DRV8825 datasheet). Use 3us.
        digitalWrite(PIN_STEP, HIGH);
        delayMicroseconds(3);
        digitalWrite(PIN_STEP, LOW);
    }

private:
    bool enabled = false;
};
