#pragma once
#include <Arduino.h>
#include <EEPROM.h>
#include "../../config/Defaults.h"

struct PersistedData {
    uint32_t magic   = 0x53464231; // "SFB1"
    uint16_t version = 5;

    MotionConfig cfg;

    uint32_t faultTotal = 0;
    uint8_t  lastFaultCode = 0;
    uint32_t lastFaultUptimeMs = 0;
    uint32_t resetCount = 0;

    // LED policy persisted settings
    uint8_t  ledMode = 0;         // 0=Auto, 1=Manual
    uint8_t  ledManualOn = 1;     // 0/1
    uint16_t ledOnStartMin = 8*60;  // default 08:00
    uint16_t ledOnEndMin   = 20*60; // default 20:00

    // Recent alert log (ring buffer, max 5)
    uint32_t alertSeq = 0;
    uint8_t  alertHead = 0;
    uint8_t  alertCount = 0;
    uint8_t  alertCodes[5] = {0};
    uint32_t alertUptimeSec[5] = {0};

    // Factory validation persisted result
    uint32_t factorySeq = 0;
    uint8_t  factoryLastPass = 0;
    uint8_t  factoryFailCode = 0;
    uint8_t  factoryFailStep = 0;
    uint32_t factoryLastDurationMs = 0;
    uint32_t factoryLastUptimeSec = 0;
    uint32_t factoryPassCount = 0;
    uint32_t factoryFailCount = 0;

    // Factory validation history log (ring buffer, max 8)
    uint8_t  factoryLogHead = 0;   // next write index
    uint8_t  factoryLogCount = 0;  // <= 8
    uint8_t  factoryLogPass[8] = {0};
    uint8_t  factoryLogFailCode[8] = {0};
    uint8_t  factoryLogFailStep[8] = {0};
    uint16_t factoryLogDurationSec[8] = {0};
    uint32_t factoryLogUptimeSec[8] = {0};
    uint32_t factoryLogCycles[8] = {0};

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
