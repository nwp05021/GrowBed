#include "modules/GrowLightModule.h"

#include <Arduino.h>

namespace growbed::modules
{

bool GrowLightModule::init()
{
    if (m_config.maxBrightnessPct > 100U) m_config.maxBrightnessPct = 100U;
    if (m_config.minBrightnessPct > 100U) m_config.minBrightnessPct = 100U;
    if (m_config.minBrightnessPct > m_config.maxBrightnessPct) {
        m_config.minBrightnessPct = m_config.maxBrightnessPct;
    }
    if (m_config.pollIntervalMs == 0U) m_config.pollIntervalMs = 1000U;

    const bool sensorOk = m_lightSensor.init();
    const bool lightOk = m_growLight.init();
    applyOff();

    Serial.printf("[GrowLightModule] init sensor=%u light=%u dimLux=%u\n",
                  sensorOk ? 1U : 0U,
                  lightOk ? 1U : 0U,
                  m_config.ambientDimLux);
    return sensorOk && lightOk;
}

void GrowLightModule::setOn(bool on)
{
    m_on = on;
    if (m_on) {
        updateBrightness();
    } else {
        applyOff();
    }
}

void GrowLightModule::tick(uint32_t nowMs)
{
    if (!m_on) return;
    if (nowMs - m_lastPollMs < m_config.pollIntervalMs) return;
    m_lastPollMs = nowMs;

    updateBrightness();
}

void GrowLightModule::updateBrightness()
{
    m_lastLux = m_lightSensor.readLux();
    m_lastBrightnessPct = computeBrightness(m_lastLux);

    m_growLight.setBrightnessPercent(m_lastBrightnessPct);
    m_growLight.setOn(m_lastBrightnessPct > 0U);
}

uint8_t GrowLightModule::computeBrightness(float lux) const
{
    if (!m_lightSensor.isConnected()) return m_config.maxBrightnessPct;
    if (m_config.ambientDimLux == 0U) return m_config.maxBrightnessPct;

    const float dimLux = static_cast<float>(m_config.ambientDimLux);
    if (lux <= dimLux) return m_config.maxBrightnessPct;
    if (lux >= dimLux * 2.0f) return m_config.minBrightnessPct;

    const float ratio = 1.0f - ((lux - dimLux) / dimLux);
    const float span = static_cast<float>(m_config.maxBrightnessPct - m_config.minBrightnessPct);
    const float pct = static_cast<float>(m_config.minBrightnessPct) + (span * ratio);

    if (pct <= 0.0f) return 0U;
    if (pct >= 100.0f) return 100U;
    return static_cast<uint8_t>(pct);
}

void GrowLightModule::applyOff()
{
    m_lastBrightnessPct = 0U;
    m_growLight.off();
}

} // namespace growbed::modules
