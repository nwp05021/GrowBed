#include "devices/RtcDriver.h"
#include "storage/NvsStorage.h"
#include <Arduino.h>
#include <hardware/rtc.h>
#include <sys/time.h>
#include <ctime>

namespace growbed::devices
{
namespace
{
    void configureTimezone()
    {
        setenv("TZ", "KST-9", 1);
        tzset();
    }

    bool epochToDatetime(uint32_t epoch, datetime_t& datetime)
    {
        const std::time_t time = static_cast<std::time_t>(epoch);
        const std::tm* local = std::localtime(&time);
        if (!local) return false;

        datetime.year = static_cast<int16_t>(local->tm_year + 1900);
        datetime.month = static_cast<int8_t>(local->tm_mon + 1);
        datetime.day = static_cast<int8_t>(local->tm_mday);
        datetime.dotw = static_cast<int8_t>(local->tm_wday);
        datetime.hour = static_cast<int8_t>(local->tm_hour);
        datetime.min = static_cast<int8_t>(local->tm_min);
        datetime.sec = static_cast<int8_t>(local->tm_sec);
        return true;
    }

    uint32_t datetimeToEpoch(const datetime_t& datetime)
    {
        std::tm local = {};
        local.tm_year = static_cast<int>(datetime.year) - 1900;
        local.tm_mon = static_cast<int>(datetime.month) - 1;
        local.tm_mday = datetime.day;
        local.tm_hour = datetime.hour;
        local.tm_min = datetime.min;
        local.tm_sec = datetime.sec;
        local.tm_isdst = -1;

        const std::time_t time = std::mktime(&local);
        return time < 0 ? 0U : static_cast<uint32_t>(time);
    }
}

bool RtcDriver::init(storage::NvsStorage& storage)
{
    configureTimezone();
    m_storage = &storage;
    rtc_init();
    m_available = true;

    uint32_t savedEpoch = 0U;
    if (m_storage->loadU32(storage::NvsStorage::kKeyRtcEpoch, savedEpoch)
        && savedEpoch >= kMinValidEpoch) {
        if (setEpoch(savedEpoch)) {
            Serial.printf("[RTC] Restored from storage. epoch=%u\n", savedEpoch);
            return true;
        }
        Serial.printf("[RTC] Stored time restore failed. epoch=%u\n", savedEpoch);
    }

    if (isValid()) {
        Serial.printf("[RTC] Pico internal RTC OK. epoch=%u\n", nowEpoch());
        return true;
    }

    Serial.println("[RTC] Pico internal RTC time is not set");
    return true;
}

bool RtcDriver::isValid() const
{
    if (!m_available) return false;
    return nowEpoch() >= kMinValidEpoch;
}

uint32_t RtcDriver::nowEpoch() const
{
    if (!m_available) return 0U;

    datetime_t datetime{};
    if (!rtc_get_datetime(&datetime)) return 0U;
    return datetimeToEpoch(datetime);
}

bool RtcDriver::setEpoch(uint32_t epoch)
{
    if (!m_available || epoch < kMinValidEpoch) return false;

    datetime_t datetime{};
    if (!epochToDatetime(epoch, datetime)) return false;
    if (!rtc_set_datetime(&datetime)) return false;

    delayMicroseconds(100);
    const uint32_t savedEpoch = nowEpoch();
    const uint32_t difference =
        savedEpoch > epoch ? savedEpoch - epoch : epoch - savedEpoch;

    if (difference > 2U) {
        Serial.printf("[RTC] Time verify failed: requested=%u actual=%u\n",
                      epoch, savedEpoch);
        return false;
    }

    if (!syncSystemTime()) {
        Serial.println("[RTC] System time sync failed after RTC update");
        return false;
    }

    if (!persist()) {
        Serial.println("[RTC] Persistent time save failed");
        return false;
    }

    Serial.printf("[RTC] Time set: epoch=%u verified=%u\n", epoch, savedEpoch);
    return true;
}

bool RtcDriver::syncSystemTime() const
{
    if (!m_available) return false;
    const uint32_t epoch = nowEpoch();
    if (epoch < kMinValidEpoch) return false;

    struct timeval tv { static_cast<time_t>(epoch), 0 };
    settimeofday(&tv, nullptr);
    configureTimezone();
    Serial.printf("[RTC] System time synced: epoch=%u (KST)\n", epoch);
    return true;
}

bool RtcDriver::persist() const
{
    if (!m_available || !m_storage) return false;
    const uint32_t epoch = nowEpoch();
    if (epoch < kMinValidEpoch) return false;

    if (!m_storage->saveU32(storage::NvsStorage::kKeyRtcEpoch, epoch)) {
        return false;
    }

    uint32_t verifiedEpoch = 0U;
    if (!m_storage->loadU32(storage::NvsStorage::kKeyRtcEpoch, verifiedEpoch)
        || verifiedEpoch != epoch) {
        Serial.printf("[RTC] Checkpoint verify failed: saved=%u read=%u\n",
                      epoch, verifiedEpoch);
        return false;
    }

    Serial.printf("[RTC] Checkpoint saved: epoch=%u\n", epoch);
    return true;
}

} // namespace growbed::devices
