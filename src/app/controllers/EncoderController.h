#pragma once
#include "../../hal/EncoderHal.h"
#include "EncoderLogic.h"

class EncoderController {
public:
    explicit EncoderController(EncoderHal& hal_);

    void begin(const EncoderConfig& cfg_);
    EncoderEvents poll();

private:
    static void isrRouter();
    void handleIsr();
    uint8_t readAB() const;

private:
    EncoderHal& hal;
    EncoderLogic logic;

    static EncoderController* instance;
};