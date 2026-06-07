#pragma once
#include <Arduino.h>
#include <cstdint>

namespace growbed::devices
{
    class StepperDriver
    {
    public:
        struct Config
        {
            int pinStep = -1;
            int pinDir = -1;
            int pinEnable = -1;
            bool invertEnable = true;
            bool invertDir = false;
        };

        explicit StepperDriver(const Config& cfg) : m_cfg(cfg) {}

        bool init();

        void enable();
        void disable();
        bool isEnabled() const { return m_enabled; }

        void setSpeed(uint32_t hz);
        uint32_t speed() const { return m_speedHz; }

        void setDirection(bool forward);
        bool dirForward() const { return m_dirForward; }

    private:
        Config m_cfg;
        bool m_initialized = false;
        bool m_enabled = false;
        bool m_dirForward = true;
        uint32_t m_speedHz = 0U;
        uint m_pwmSlice = 0U;
        uint m_pwmChannel = 0U;

        void updatePulseOutput();
    };
}
