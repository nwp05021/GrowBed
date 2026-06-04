#pragma once

#include <cstdint>

namespace growbed::devices
{
    class GrowLightDriver
    {
    public:
        struct Config
        {
            int      pin = -1;
            uint32_t pwmFreqHz = 1000U;
            uint16_t pwmRange = 255U;
            uint8_t  initialBrightnessPct = 100U;
            bool     invertOutput = false;
        };

        explicit GrowLightDriver(int pin) : m_config() { m_config.pin = pin; }
        explicit GrowLightDriver(const Config& config) : m_config(config) {}

        bool init();

        void setBrightnessPercent(uint8_t percent);
        uint8_t brightnessPercent() const { return m_brightnessPct; }

        void setOn(bool on);
        void on() { setOn(true); }
        void off() { setOn(false); }
        void toggle() { setOn(!m_on); }

        bool isOn() const { return m_on; }

    private:
        Config m_config;
        bool   m_on = false;
        uint8_t m_brightnessPct = 0U;

        void applyOutput();
    };
}
