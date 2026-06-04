#pragma once
#include <Arduino.h>
#include <cstdint>

namespace growbed::devices
{
    class GpioOutput
    {
    public:
        explicit GpioOutput(int pin, bool invertLogic = false)
            : m_pin(pin), m_invert(invertLogic) {}

        bool init()
        {
            if (m_pin < 0) { m_state = false; return true; }
            pinMode(m_pin, OUTPUT);
            digitalWrite(m_pin, m_invert ? HIGH : LOW);
            return true;
        }

        void set(bool on)
        {
            if (m_pin < 0) { m_state = false; return; }
            m_state = on;
            digitalWrite(m_pin, (on ^ m_invert) ? HIGH : LOW);
        }

        bool isOn()   const { return m_state; }
        void on()           { set(true); }
        void off()          { set(false); }
        void toggle()       { set(!m_state); }

    private:
        int  m_pin;
        bool m_invert = false;
        bool m_state  = false;
    };
}
