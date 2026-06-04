#include "devices/GrowLightDriver.h"

#include <Arduino.h>

namespace growbed::devices
{

bool GrowLightDriver::init()
{
    if (m_config.pin < 0 || m_config.pwmRange == 0U) {
        m_on = false;
        m_brightnessPct = 0U;
        Serial.println("[GrowLight] invalid config");
        return false;
    }

    if (m_config.initialBrightnessPct > 100U) {
        m_config.initialBrightnessPct = 100U;
    }

    analogWriteFreq(m_config.pwmFreqHz);
    analogWriteRange(m_config.pwmRange);
    pinMode(m_config.pin, OUTPUT);

    m_brightnessPct = m_config.initialBrightnessPct;
    m_on = false;
    applyOutput();

    Serial.printf("[GrowLight] OK (pin=%d freq=%luHz)\n",
                  m_config.pin,
                  static_cast<unsigned long>(m_config.pwmFreqHz));
    return true;
}

void GrowLightDriver::setBrightnessPercent(uint8_t percent)
{
    if (percent > 100U) percent = 100U;
    m_brightnessPct = percent;
    applyOutput();
}

void GrowLightDriver::setOn(bool on)
{
    m_on = on;
    applyOutput();
}

void GrowLightDriver::applyOutput()
{
    if (m_config.pin < 0 || m_config.pwmRange == 0U) return;

    const uint8_t outputPct = m_on ? m_brightnessPct : 0U;
    uint32_t pwmValue = static_cast<uint32_t>(outputPct) *
                        static_cast<uint32_t>(m_config.pwmRange) / 100U;
    if (m_config.invertOutput) {
        pwmValue = static_cast<uint32_t>(m_config.pwmRange) - pwmValue;
    }

    analogWrite(m_config.pin, static_cast<int>(pwmValue));
}

} // namespace growbed::devices
