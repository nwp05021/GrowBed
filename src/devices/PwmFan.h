#pragma once
#include <Arduino.h>
#include <cstdint>

namespace growbed::devices
{
    class PwmFan
    {
    public:
        // channel ?¸ìˆ˜??ESP32 ?¸í™˜??? ì?ë¥??„í•´ ë°›ì?ë§??¬ìš©?˜ì? ?ŠìŒ
        PwmFan(int pin, int /*channel*/ = 0, uint32_t freqHz = 25000U)
            : m_pin(pin), m_freq(freqHz) {}

        bool init()
        {
            if (m_pin < 0) { m_duty = 0; return true; }
            analogWriteFreq(m_freq);
            analogWriteRange(255);
            pinMode(m_pin, OUTPUT);
            analogWrite(m_pin, 0);
            return true;
        }

        void setDuty(uint8_t percent)
        {
            if (m_pin < 0) { m_duty = 0; return; }
            if (percent > 100U) percent = 100U;
            m_duty = percent;
            uint8_t val = static_cast<uint8_t>(static_cast<uint32_t>(percent) * 255U / 100U);
            analogWrite(m_pin, val);
        }

        uint8_t getDuty() const { return m_duty; }
        void stop() { setDuty(0); }

    private:
        int      m_pin;
        uint32_t m_freq;
        uint8_t  m_duty = 0;
    };
}
