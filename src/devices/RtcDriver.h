#pragma once
#include <cstdint>

namespace growbed::storage
{
    class NvsStorage;
}

// Raspberry Pi Pico internal RTC driver.
namespace growbed::devices
{
    class RtcDriver
    {
    public:
        bool init(storage::NvsStorage& storage);
        bool isValid() const;
        uint32_t nowEpoch() const;
        bool setEpoch(uint32_t epoch);
        bool syncSystemTime() const;
        bool persist() const;

        bool isAvailable() const { return m_available; }

    private:
        bool m_available = false;
        storage::NvsStorage* m_storage = nullptr;

        static constexpr uint32_t kMinValidEpoch = 1577836800U; // 2020-01-01 UTC
    };
}
