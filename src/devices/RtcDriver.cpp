#include "devices/RtcDriver.h"
#include <Arduino.h>
#include <sys/time.h>  // settimeofday()

namespace growbed::devices
{

bool RtcDriver::init()
{
    if (!m_rtc.begin()) {
        Serial.println("[RTC] DS3231 not found on I2C bus");
        m_available = false;
        return false;
    }

    m_available = true;

    if (m_rtc.lostPower()) {
        Serial.println("[RTC] Power was lost ??time is invalid. Set via UI.");
    } else {
        Serial.printf("[RTC] DS3231 OK. epoch=%u\n", nowEpoch());
    }
    return true;
}

bool RtcDriver::isValid() const
{
    if (!m_available) return false;
    if (m_rtc.lostPower()) return false;
    return nowEpoch() >= kMinValidEpoch;
}

uint32_t RtcDriver::nowEpoch() const
{
    if (!m_available) return 0U;
    return static_cast<uint32_t>(m_rtc.now().unixtime());
}

bool RtcDriver::setEpoch(uint32_t epoch)
{
    if (!m_available) return false;
    m_rtc.adjust(DateTime(epoch));
    syncSystemTime();
    Serial.printf("[RTC] Time set: epoch=%u\n", epoch);
    return true;
}

bool RtcDriver::syncSystemTime() const
{
    if (!m_available) return false;
    uint32_t ep = nowEpoch();
    if (ep < kMinValidEpoch) return false;

    struct timeval tv { static_cast<time_t>(ep), 0 };
    settimeofday(&tv, nullptr);

    // ?œêµ­ ?œì????¤ì •
    setenv("TZ", "KST-9", 1);
    tzset();

    Serial.printf("[RTC] System time synced: epoch=%u (KST)\n", ep);
    return true;
}

} // namespace growbed::devices
