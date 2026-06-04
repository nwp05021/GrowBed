#pragma once

#include <cstdint>

namespace growbed::devices
{
    class HallSensorDriver
    {
    public:
        struct Config
        {
            int      pin = -1;
            bool     activeLow = true;
            bool     usePullup = true;
            uint32_t debounceMs = 5U;
        };

        explicit HallSensorDriver(int pin) : m_config() { m_config.pin = pin; }
        explicit HallSensorDriver(const Config& config) : m_config(config) {}

        bool init();
        bool isConnected() const { return m_connected; }

        void tick(uint32_t nowMs);

        bool isDetected() const { return m_stableDetected; }
        bool rawDetected() const;

        bool rose();
        bool fell();
        bool changed();

        uint32_t lastChangeMs() const { return m_lastStableChangeMs; }

    private:
        Config   m_config;
        bool     m_connected = false;
        bool     m_stableDetected = false;
        bool     m_lastRawDetected = false;
        bool     m_pendingRose = false;
        bool     m_pendingFall = false;
        uint32_t m_lastRawChangeMs = 0;
        uint32_t m_lastStableChangeMs = 0;
    };
}
