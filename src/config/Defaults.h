
#pragma once
#include <stdint.h>

// 24/365 안정성 우선: 보수적 기본값
struct MotionConfig {
    float maxSps = 3000.0f;       // max speed (steps/sec)
    float minSps = 12000.0f;      // min speed during homing / crawl
    float accel  = 6000.0f;       // acceleration (steps/sec^2)
    uint32_t dwellMs = 300;      // dwell at each end
    uint32_t homingTimeoutMs = 15000;
    uint32_t travelTimeoutMs = 25000; // fallback if travelSteps not learned yet
    uint32_t rehomeEveryCycles = 200; // full cycles (L->R->L) then rehome
};

struct UiConfig {
    uint32_t refreshMs = 100;   // OLED redraw tick
    uint32_t sensorMs  = 1000;  // AHT read tick
};

struct EncoderConfig {
    uint32_t btnDebounceMs = 30;
    uint32_t longPressMs = 1500;
    uint32_t veryLongPressMs = 5000; // Engineering mode entry
};
