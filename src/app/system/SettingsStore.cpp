#include "SettingsStore.h"

uint32_t SettingsStore::calcCRC(const PersistedData& d) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&d);
    uint32_t crc = 0;
    // exclude the crc field itself
    for (size_t i = 0; i < sizeof(PersistedData) - sizeof(uint32_t); i++) {
        crc = (crc * 33u) ^ p[i];
    }
    return crc;
}

bool SettingsStore::load(PersistedData& out) {
    // Backward-compatible load for v1 -> v2 -> v3 migration.
    struct PersistedDataV1 {
        uint32_t magic   = 0x53464231;
        uint16_t version = 1;
        MotionConfig cfg;
        uint32_t faultTotal = 0;
        uint8_t  lastFaultCode = 0;
        uint32_t lastFaultUptimeMs = 0;
        uint32_t resetCount = 0;
        uint32_t crc = 0;
    };

    struct PersistedDataV2 {
        uint32_t magic   = 0x53464231;
        uint16_t version = 2;
        MotionConfig cfg;
        uint32_t faultTotal = 0;
        uint8_t  lastFaultCode = 0;
        uint32_t lastFaultUptimeMs = 0;
        uint32_t resetCount = 0;
        uint8_t  ledMode = 0;
        uint8_t  ledManualOn = 1;
        uint16_t ledOnStartMin = 8*60;
        uint16_t ledOnEndMin   = 20*60;
        uint32_t crc = 0;
    };

    uint32_t magic = 0;
    uint16_t version = 0;
    EEPROM.get(0, magic);
    EEPROM.get(sizeof(uint32_t), version);

    if (magic != 0x53464231) return false;

    if (version == 1) {
        PersistedDataV1 v1;
        EEPROM.get(0, v1);
        // CRC for v1 struct size
        const uint8_t* p = reinterpret_cast<const uint8_t*>(&v1);
        uint32_t crc = 0;
        for (size_t i = 0; i < sizeof(PersistedDataV1) - sizeof(uint32_t); i++) {
            crc = (crc * 33u) ^ p[i];
        }
        if (crc != v1.crc) return false;

        out = PersistedData{};
        out.magic = v1.magic;
        out.version = 5;
        out.cfg = v1.cfg;
        out.faultTotal = v1.faultTotal;
        out.lastFaultCode = v1.lastFaultCode;
        out.lastFaultUptimeMs = v1.lastFaultUptimeMs;
        out.resetCount = v1.resetCount;
        // LED defaults are already set in PersistedData
        return true;
    }

    if (version == 2) {
        PersistedDataV2 v2;
        EEPROM.get(0, v2);
        const uint8_t* p = reinterpret_cast<const uint8_t*>(&v2);
        uint32_t crc = 0;
        for (size_t i = 0; i < sizeof(PersistedDataV2) - sizeof(uint32_t); i++) {
            crc = (crc * 33u) ^ p[i];
        }
        if (crc != v2.crc) return false;

        out = PersistedData{};
        out.magic = v2.magic;
        out.version = 5;
        out.cfg = v2.cfg;
        out.faultTotal = v2.faultTotal;
        out.lastFaultCode = v2.lastFaultCode;
        out.lastFaultUptimeMs = v2.lastFaultUptimeMs;
        out.resetCount = v2.resetCount;
        out.ledMode = v2.ledMode;
        out.ledManualOn = v2.ledManualOn;
        out.ledOnStartMin = v2.ledOnStartMin;
        out.ledOnEndMin = v2.ledOnEndMin;
        // alert log defaults already set
        return true;
    }

        // v3 -> v4 migration (adds factory fields)
    if (version == 3) {
        struct PersistedDataV3 {
            uint32_t magic   = 0x53464231;
            uint16_t version = 3;
            MotionConfig cfg;
            uint32_t faultTotal = 0;
            uint8_t  lastFaultCode = 0;
            uint32_t lastFaultUptimeMs = 0;
            uint32_t resetCount = 0;
            uint8_t  ledMode = 0;
            uint8_t  ledManualOn = 1;
            uint16_t ledOnStartMin = 8*60;
            uint16_t ledOnEndMin   = 20*60;
            uint32_t alertSeq = 0;
            uint8_t  alertHead = 0;
            uint8_t  alertCount = 0;
            uint8_t  alertCodes[5] = {0};
            uint32_t alertUptimeSec[5] = {0};
            uint32_t crc = 0;
        };

        PersistedDataV3 v3;
        EEPROM.get(0, v3);
        const uint8_t* p = reinterpret_cast<const uint8_t*>(&v3);
        uint32_t crc = 0;
        for (size_t i = 0; i < sizeof(PersistedDataV3) - sizeof(uint32_t); i++) {
            crc = (crc * 33u) ^ p[i];
        }
        if (crc != v3.crc) return false;

        out = PersistedData{};
        out.magic = v3.magic;
        out.version = 5;
        out.cfg = v3.cfg;
        out.faultTotal = v3.faultTotal;
        out.lastFaultCode = v3.lastFaultCode;
        out.lastFaultUptimeMs = v3.lastFaultUptimeMs;
        out.resetCount = v3.resetCount;
        out.ledMode = v3.ledMode;
        out.ledManualOn = v3.ledManualOn;
        out.ledOnStartMin = v3.ledOnStartMin;
        out.ledOnEndMin = v3.ledOnEndMin;
        out.alertSeq = v3.alertSeq;
        out.alertHead = v3.alertHead;
        out.alertCount = v3.alertCount;
        for (uint8_t i = 0; i < 5; i++) {
            out.alertCodes[i] = v3.alertCodes[i];
            out.alertUptimeSec[i] = v3.alertUptimeSec[i];
        }
        // factory fields remain defaults
        return true;
    }

    if (version == 4) {
        // v4 -> v5 migration (adds factory log ring buffer)
        struct PersistedDataV4 {
            uint32_t magic   = 0x53464231;
            uint16_t version = 4;
            MotionConfig cfg;
            uint32_t faultTotal = 0;
            uint8_t  lastFaultCode = 0;
            uint32_t lastFaultUptimeMs = 0;
            uint32_t resetCount = 0;
            uint8_t  ledMode = 0;
            uint8_t  ledManualOn = 1;
            uint16_t ledOnStartMin = 8*60;
            uint16_t ledOnEndMin   = 20*60;
            uint32_t alertSeq = 0;
            uint8_t  alertHead = 0;
            uint8_t  alertCount = 0;
            uint8_t  alertCodes[5] = {0};
            uint32_t alertUptimeSec[5] = {0};
            uint32_t factorySeq = 0;
            uint8_t  factoryLastPass = 0;
            uint8_t  factoryFailCode = 0;
            uint8_t  factoryFailStep = 0;
            uint32_t factoryLastDurationMs = 0;
            uint32_t factoryLastUptimeSec = 0;
            uint32_t factoryPassCount = 0;
            uint32_t factoryFailCount = 0;
            uint32_t crc = 0;
        };
        PersistedDataV4 v4;
        EEPROM.get(0, v4);
        // CRC for v4 struct size
        const uint8_t* p4 = reinterpret_cast<const uint8_t*>(&v4);
        uint32_t crc4 = 0;
        for (size_t i = 0; i < sizeof(PersistedDataV4) - sizeof(uint32_t); i++) {
            crc4 = (crc4 * 33u) ^ p4[i];
        }
        if (crc4 != v4.crc) return false;
        out = PersistedData{};
        // copy whole v4 blob into v5-compatible struct field-by-field
        out.magic = v4.magic;
        out.version = 5;
        out.cfg = v4.cfg;
        out.faultTotal = v4.faultTotal;
        out.lastFaultCode = v4.lastFaultCode;
        out.lastFaultUptimeMs = v4.lastFaultUptimeMs;
        out.resetCount = v4.resetCount;
        out.ledMode = v4.ledMode;
        out.ledManualOn = v4.ledManualOn;
        out.ledOnStartMin = v4.ledOnStartMin;
        out.ledOnEndMin = v4.ledOnEndMin;
        out.alertSeq = v4.alertSeq;
        out.alertHead = v4.alertHead;
        out.alertCount = v4.alertCount;
        for (uint8_t i = 0; i < 5; i++) { out.alertCodes[i] = v4.alertCodes[i]; out.alertUptimeSec[i] = v4.alertUptimeSec[i]; }
        out.factorySeq = v4.factorySeq;
        out.factoryLastPass = v4.factoryLastPass;
        out.factoryFailCode = v4.factoryFailCode;
        out.factoryFailStep = v4.factoryFailStep;
        out.factoryLastDurationMs = v4.factoryLastDurationMs;
        out.factoryLastUptimeSec = v4.factoryLastUptimeSec;
        out.factoryPassCount = v4.factoryPassCount;
        out.factoryFailCount = v4.factoryFailCount;
        // factory log defaults remain empty
        return true;
    }

    if (version != 5) return false;

    EEPROM.get(0, out);
    return (calcCRC(out) == out.crc);
}

void SettingsStore::save(PersistedData in) {
    in.crc = calcCRC(in);
    EEPROM.put(0, in);
    EEPROM.commit();
}
