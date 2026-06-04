#include "modules/GrowManager.h"

#include <Arduino.h>
#include <ctime>

namespace growbed::modules
{

bool GrowManager::init()
{
    if (m_config.startHour > 23U) m_config.startHour = 23U;
    if (m_config.endHour > 23U) m_config.endHour = 23U;
    if (m_config.startMinute > 59U) m_config.startMinute = 59U;
    if (m_config.endMinute > 59U) m_config.endMinute = 59U;
    if (m_config.pollIntervalMs == 0U) m_config.pollIntervalMs = 1000U;

    applyActive(false);
    Serial.printf("[GrowManager] schedule=%02u:%02u-%02u:%02u\n",
                  m_config.startHour,
                  m_config.startMinute,
                  m_config.endHour,
                  m_config.endMinute);
    return true;
}

void GrowManager::tick(uint32_t nowMs)
{
    if (nowMs - m_lastPollMs < m_config.pollIntervalMs) return;
    m_lastPollMs = nowMs;

    applyActive(scheduleAllowsGrow());
}

bool GrowManager::scheduleAllowsGrow() const
{
    const uint16_t startMinuteOfDay =
        static_cast<uint16_t>(m_config.startHour) * 60U + m_config.startMinute;
    const uint16_t endMinuteOfDay =
        static_cast<uint16_t>(m_config.endHour) * 60U + m_config.endMinute;

    if (startMinuteOfDay == endMinuteOfDay) return true;

    std::time_t now = std::time(nullptr);
    std::tm* tmv = std::localtime(&now);
    if (!tmv) return false;

    const uint16_t currentMinuteOfDay =
        static_cast<uint16_t>(tmv->tm_hour) * 60U + static_cast<uint16_t>(tmv->tm_min);

    if (startMinuteOfDay < endMinuteOfDay) {
        return currentMinuteOfDay >= startMinuteOfDay &&
               currentMinuteOfDay < endMinuteOfDay;
    }

    return currentMinuteOfDay >= startMinuteOfDay ||
           currentMinuteOfDay < endMinuteOfDay;
}

void GrowManager::applyActive(bool active)
{
    if (m_active == active &&
        m_lightModule.isOn() == active &&
        m_trayModule.isOn() == active) {
        return;
    }

    m_active = active;
    m_lightModule.setOn(active);
    m_trayModule.setOn(active);

    Serial.printf("[GrowManager] %s\n", active ? "on" : "off");
}

} // namespace growbed::modules
