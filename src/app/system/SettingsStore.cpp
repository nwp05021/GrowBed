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
    EEPROM.get(0, out);
    if (out.magic != 0x53464231) return false;
    if (out.version != 1) return false;
    return (calcCRC(out) == out.crc);
}

void SettingsStore::save(PersistedData in) {
    in.crc = calcCRC(in);
    EEPROM.put(0, in);
    EEPROM.commit();
}
