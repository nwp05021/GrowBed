#pragma once

#include "devices/CdsGl5537Driver.h"
#include "devices/GrowLightDriver.h"
#include <cstdint>

namespace growbed::modules
{
    class GrowLightModule
    {
    public:
        struct Config
        {
            uint8_t  maxBrightnessPct = 80U;
            uint8_t  minBrightnessPct = 0U;
            uint16_t ambientDimLux = 3000U;
            uint32_t pollIntervalMs = 5000U;
        };

        GrowLightModule(devices::CdsGl5537Driver& lightSensor,
                        devices::GrowLightDriver& growLight)
            : GrowLightModule(lightSensor, growLight, Config{}) {}

        GrowLightModule(devices::CdsGl5537Driver& lightSensor,
                        devices::GrowLightDriver& growLight,
                        const Config& config)
            : m_lightSensor(lightSensor),
              m_growLight(growLight),
              m_config(config) {}

        bool init();
        void tick(uint32_t nowMs);

        void setOn(bool on);
        void on() { setOn(true); }
        void off() { setOn(false); }

        bool isOn() const { return m_on; }
        float ambientLux() const { return m_lastLux; }
        uint8_t brightnessPercent() const { return m_lastBrightnessPct; }

    private:
        devices::CdsGl5537Driver& m_lightSensor;
        devices::GrowLightDriver& m_growLight;
        Config m_config;

        bool     m_on = false;
        float    m_lastLux = 0.0f;
        uint8_t  m_lastBrightnessPct = 0U;
        uint32_t m_lastPollMs = 0U;

        void updateBrightness();
        uint8_t computeBrightness(float lux) const;
        void applyOff();
    };
}
