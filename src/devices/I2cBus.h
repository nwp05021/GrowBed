#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <cstdint>

namespace growbed::devices
{
    class I2cBus
    {
    public:
        bool init(int sda, int scl, uint32_t freqHz = 400000U)
        {
            Wire.setSDA(sda);
            Wire.setSCL(scl);
            Wire.begin();
            Wire.setClock(freqHz);
            return true;
        }
    };
}
