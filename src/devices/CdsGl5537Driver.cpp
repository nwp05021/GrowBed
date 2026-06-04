#include "devices/CdsGl5537Driver.h"

#include <Arduino.h>
#include <cmath>

namespace growbed::devices
{

bool CdsGl5537Driver::init()
{
    if (m_config.pin < 0 || m_config.adcMax == 0U ||
        m_config.seriesResistorOhms == 0U || m_config.referenceVoltage <= 0.0f) {
        m_connected = false;
        Serial.println("[CDS GL5537] invalid config");
        return false;
    }

    analogReadResolution(m_config.adcResolutionBits);
    pinMode(m_config.pin, INPUT);
    m_connected = true;
    Serial.printf("[CDS GL5537] OK (pin=%d)\n", m_config.pin);
    return true;
}

uint16_t CdsGl5537Driver::readRawSampled() const
{
    const uint8_t samples = (m_config.samples == 0U) ? 1U : m_config.samples;
    uint32_t sum = 0;
    for (uint8_t i = 0; i < samples; ++i) {
        sum += static_cast<uint32_t>(analogRead(m_config.pin));
    }
    return static_cast<uint16_t>(sum / samples);
}

uint16_t CdsGl5537Driver::readRaw()
{
    if (!m_connected) return 0U;

    uint16_t raw = readRawSampled();
    if (raw > m_config.adcMax) raw = m_config.adcMax;
    return raw;
}

float CdsGl5537Driver::readVoltage()
{
    const uint16_t raw = readRaw();
    if (!m_connected) return 0.0f;
    return (static_cast<float>(raw) * m_config.referenceVoltage) /
           static_cast<float>(m_config.adcMax);
}

float CdsGl5537Driver::readResistanceOhms()
{
    const uint16_t raw = readRaw();
    if (!m_connected || raw == 0U || raw >= m_config.adcMax) return 0.0f;

    const float ratio = static_cast<float>(raw) /
                        static_cast<float>(m_config.adcMax - raw);
    const float fixed = static_cast<float>(m_config.seriesResistorOhms);

    if (m_config.sensorToVcc) {
        return fixed / ratio;
    }
    return fixed * ratio;
}

float CdsGl5537Driver::readLux()
{
    const float resistance = readResistanceOhms();
    if (!m_connected || resistance <= 0.0f || m_config.luxAt10kOhms <= 0.0f ||
        m_config.gamma <= 0.0f) {
        return 0.0f;
    }

    const float normalized = 10000.0f / resistance;
    const float lux = m_config.luxAt10kOhms * std::pow(normalized, 1.0f / m_config.gamma);
    return (lux < 0.0f) ? 0.0f : lux;
}

uint8_t CdsGl5537Driver::readBrightnessPercent()
{
    const uint16_t raw = readRaw();
    if (!m_connected) return 0U;

    uint32_t percent = static_cast<uint32_t>(raw) * 100U / m_config.adcMax;
    if (!m_config.sensorToVcc) percent = 100U - percent;
    if (percent > 100U) percent = 100U;
    return static_cast<uint8_t>(percent);
}

} // namespace growbed::devices
