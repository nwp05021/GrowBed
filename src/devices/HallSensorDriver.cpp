#include "devices/HallSensorDriver.h"

#include <Arduino.h>

namespace growbed::devices
{

bool HallSensorDriver::init()
{
    if (m_config.pin <= 0) {
        m_connected = false;
        Serial.println("[HallSensor] invalid pin");
        return false;
    }

    pinMode(m_config.pin, m_config.usePullup ? INPUT_PULLUP : INPUT);

    const uint32_t nowMs = millis();
    m_lastRawDetected = rawDetected();
    m_stableDetected = m_lastRawDetected;
    m_lastRawChangeMs = nowMs;
    m_lastStableChangeMs = nowMs;
    m_pendingRose = false;
    m_pendingFall = false;
    m_connected = true;

    Serial.printf("[HallSensor] OK (pin=%d activeLow=%u pullup=%u)\n",
                  m_config.pin,
                  m_config.activeLow ? 1U : 0U,
                  m_config.usePullup ? 1U : 0U);
    return true;
}

bool HallSensorDriver::rawDetected() const
{
    if (!m_connected && m_config.pin < 0) return false;

    const bool high = (digitalRead(m_config.pin) == HIGH);
    return m_config.activeLow ? !high : high;
}

void HallSensorDriver::tick(uint32_t nowMs)
{
    if (!m_connected) return;

    const bool raw = rawDetected();
    if (raw != m_lastRawDetected) {
        m_lastRawDetected = raw;
        m_lastRawChangeMs = nowMs;
    }

    if (raw == m_stableDetected) return;
    if (nowMs - m_lastRawChangeMs < m_config.debounceMs) return;

    m_stableDetected = raw;
    m_lastStableChangeMs = nowMs;
    if (m_stableDetected) {
        m_pendingRose = true;
    } else {
        m_pendingFall = true;
    }
}

bool HallSensorDriver::rose()
{
    if (!m_pendingRose) return false;
    m_pendingRose = false;
    return true;
}

bool HallSensorDriver::fell()
{
    if (!m_pendingFall) return false;
    m_pendingFall = false;
    return true;
}

bool HallSensorDriver::changed()
{
    const bool any = m_pendingRose || m_pendingFall;
    m_pendingRose = false;
    m_pendingFall = false;
    return any;
}

} // namespace growbed::devices
