#pragma once

#include <cstdint>
#include "config/PinConfig.h"

namespace growbed::devices
{
    class CdsGl5537Driver
    {
    public:
        struct Config
        {
            int      pin = growbed::config::Pin::CDS_GL5537;
            uint32_t seriesResistorOhms = 10000U;
            float    referenceVoltage = 3.3f;
            uint16_t adcMax = 4095U;
            uint8_t  adcResolutionBits = 12U;
            bool     sensorToVcc = true;
            float    luxAt10kOhms = 10.0f;
            float    gamma = 0.8f;
            uint8_t  samples = 8U;
        };

        explicit CdsGl5537Driver(const Config& config) : m_config(config) {}
        explicit CdsGl5537Driver(int pin) : m_config() { m_config.pin = pin; }

        bool init();
        bool isConnected() const { return m_connected; }

        uint16_t readRaw();
        float readVoltage();
        float readResistanceOhms();
        float readLux();
        uint8_t readBrightnessPercent();

        const Config& config() const { return m_config; }

    private:
        Config m_config;
        bool   m_connected = false;

        uint16_t readRawSampled() const;
    };
}
