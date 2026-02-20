#pragma once

#include <Arduino.h>

#include "../controllers/EncoderController.h"

class MotionController;
struct UiConfig;

// UiController owns the UI state machine and converts encoder events into motion requests.
// It must NEVER directly mutate MotionStatus/state (MotionController is the sole owner).
class UiController {
public:
    UiController();
    ~UiController();

    void begin(const UiConfig& cfg, MotionController* motion);
    void handleEncoder(const EncoderEvents& e);
    void tick();

private:
    struct Impl;
    Impl* _;
};
