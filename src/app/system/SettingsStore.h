#pragma once
#include <Arduino.h>
#include <EEPROM.h>
#include "../../config/Defaults.h"

struct PersistedData {
    uint32_t magic   = 0x53464231; // "SFB1"
    uint16_t version = 1;

    MotionConfig cfg;

    uint32_t faultTotal = 0;
    uint8_t  lastFaultCode = 0;
    uint32_t lastFaultUptimeMs = 0;
    uint32_t resetCount = 0;

    uint32_t crc = 0;
};

class SettingsStore {
public:
    static constexpr size_t EEPROM_SIZE = 1024;

    void begin() { EEPROM.begin(EEPROM_SIZE); }
    bool load(PersistedData& out);
    void save(PersistedData in);

private:
    uint32_t calcCRC(const PersistedData& d);
};
