#pragma once

#include "I2cBus.h"
#include <Arduino.h>
#include <cstddef>
#include <cstdint>

namespace growbed::devices
{
    class Aht20Driver
    {
    public:
        explicit Aht20Driver(I2cBus& /*bus*/) {}

        bool init()
        {
            m_connected = false;
            m_cacheValid = false;

            if (!writeCommand(kCmdSoftReset)) {
                Serial.println("[AHT20] not found on I2C bus");
                return false;
            }
            delay(20);

            uint8_t status = 0;
            if (!readStatus(status)) {
                Serial.println("[AHT20] status read failed");
                return false;
            }

            if ((status & kStatusCalibrated) == 0U) {
                const uint8_t initCmd[] = {kCmdInit, 0x08U, 0x00U};
                if (!writeBytes(initCmd, sizeof(initCmd))) {
                    Serial.println("[AHT20] calibration command failed");
                    return false;
                }
                delay(10);
            }

            m_connected = true;
            Serial.println("[AHT20] OK");
            return true;
        }

        bool isConnected() const { return m_connected; }

        bool triggerMeasurement()
        {
            uint8_t status = 0;
            if (!readStatus(status)) {
                m_connected = false;
                m_cacheValid = false;
                return false;
            }
            if ((status & kStatusBusy) != 0U) return false;

            const uint8_t cmd[] = {kCmdTrigger, 0x33U, 0x00U};
            const bool ok = writeBytes(cmd, sizeof(cmd));
            m_connected = ok;
            if (!ok) m_cacheValid = false;
            return ok;
        }

        bool fetchResult()
        {
            uint8_t data[7] = {};
            const uint8_t received = Wire.requestFrom(kAddress, static_cast<uint8_t>(sizeof(data)));
            if (received < 6U) {
                m_connected = false;
                m_cacheValid = false;
                return false;
            }

            for (uint8_t i = 0; i < received && i < sizeof(data); ++i) {
                data[i] = static_cast<uint8_t>(Wire.read());
            }

            if ((data[0] & kStatusBusy) != 0U) {
                m_cacheValid = false;
                return false;
            }

            const uint32_t rawHumi =
                (static_cast<uint32_t>(data[1]) << 12) |
                (static_cast<uint32_t>(data[2]) << 4) |
                (static_cast<uint32_t>(data[3]) >> 4);
            const uint32_t rawTemp =
                ((static_cast<uint32_t>(data[3]) & 0x0FU) << 16) |
                (static_cast<uint32_t>(data[4]) << 8) |
                static_cast<uint32_t>(data[5]);

            m_cachedHumi = (static_cast<float>(rawHumi) * 100.0f) / 1048576.0f;
            m_cachedTemp = (static_cast<float>(rawTemp) * 200.0f) / 1048576.0f - 50.0f;

            if (m_cachedHumi < 0.0f) m_cachedHumi = 0.0f;
            if (m_cachedHumi > 100.0f) m_cachedHumi = 100.0f;

            m_connected = true;
            m_cacheValid = true;
            return true;
        }

        bool isCacheValid() const { return m_cacheValid; }
        float getCachedTemp() const { return m_cachedTemp; }
        float getCachedHumi() const { return m_cachedHumi; }

    private:
        static constexpr uint8_t kAddress = 0x38U;
        static constexpr uint8_t kCmdInit = 0xBEU;
        static constexpr uint8_t kCmdSoftReset = 0xBAU;
        static constexpr uint8_t kCmdTrigger = 0xACU;
        static constexpr uint8_t kStatusBusy = 0x80U;
        static constexpr uint8_t kStatusCalibrated = 0x08U;

        bool m_connected = false;
        bool m_cacheValid = false;
        float m_cachedTemp = 0.0f;
        float m_cachedHumi = 0.0f;

        bool readStatus(uint8_t& status)
        {
            if (Wire.requestFrom(kAddress, static_cast<uint8_t>(1)) != 1U) return false;
            status = static_cast<uint8_t>(Wire.read());
            return true;
        }

        bool writeCommand(uint8_t command)
        {
            Wire.beginTransmission(kAddress);
            Wire.write(command);
            return Wire.endTransmission() == 0;
        }

        bool writeBytes(const uint8_t* data, size_t length)
        {
            Wire.beginTransmission(kAddress);
            Wire.write(data, length);
            return Wire.endTransmission() == 0;
        }
    };
}
