#pragma once

#include <Arduino.h>
#include <cstddef>
#include <cstdint>

namespace growbed::devices
{
    class Rs485Driver
    {
    public:
        struct Config
        {
            int      pinTx = 0;
            int      pinRx = 1;
            int      pinDe = 13;
            int      pinRe = -1;
            uint32_t baud  = 9600U;
        };

        explicit Rs485Driver(SerialUART& serial) : m_serial(serial) {}

        bool init(const Config& cfg);
        size_t write(const uint8_t* data, size_t len);
        size_t writeText(const char* text);
        int available() const;
        int read();
        void flushInput();
        bool isReady() const { return m_ready; }

    private:
        SerialUART& m_serial;
        Config m_cfg{};
        bool m_ready = false;

        void setTransmitMode(bool tx);
    };
}
