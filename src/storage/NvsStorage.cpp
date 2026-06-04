#include "storage/NvsStorage.h"
#include <LittleFS.h>
#include <Arduino.h>
#include <cstdio>
#include <cstring>

namespace growbed::storage
{

static constexpr const char* kNvsDir = "/nvs";

void NvsStorage::buildPath(char* out, size_t outLen, const char* key) const
{
    std::snprintf(out, outLen, "%s/%s.bin", kNvsDir, key);
}

bool NvsStorage::init()
{
    if (!LittleFS.begin()) {
        Serial.println("[NvsStorage] LittleFS mount failed");
        return false;
    }
    if (!LittleFS.exists(kNvsDir)) {
        LittleFS.mkdir(kNvsDir);
    }
    m_open = true;
    return true;
}

bool NvsStorage::saveBlob(const char* key, const void* data, size_t size)
{
    if (!m_open) return false;
    char path[64];
    buildPath(path, sizeof(path), key);
    File f = LittleFS.open(path, "w");
    if (!f) {
        Serial.printf("[NvsStorage] saveBlob open failed: %s\n", path);
        return false;
    }
    size_t written = f.write(static_cast<const uint8_t*>(data), size);
    f.close();
    return (written == size);
}

bool NvsStorage::loadBlob(const char* key, void* data, size_t expectedSize)
{
    if (!m_open) return false;
    char path[64];
    buildPath(path, sizeof(path), key);
    if (!LittleFS.exists(path)) return false;
    File f = LittleFS.open(path, "r");
    if (!f) return false;
    if (static_cast<size_t>(f.size()) != expectedSize) {
        f.close();
        Serial.printf("[NvsStorage] loadBlob size mismatch: %s\n", key);
        return false;
    }
    size_t read = f.read(static_cast<uint8_t*>(data), expectedSize);
    f.close();
    return (read == expectedSize);
}

bool NvsStorage::saveU32(const char* key, uint32_t value)
{
    return saveBlob(key, &value, sizeof(value));
}

bool NvsStorage::loadU32(const char* key, uint32_t& outValue)
{
    return loadBlob(key, &outValue, sizeof(outValue));
}

bool NvsStorage::eraseKey(const char* key)
{
    if (!m_open) return false;
    char path[64];
    buildPath(path, sizeof(path), key);
    if (!LittleFS.exists(path)) return true;
    return LittleFS.remove(path);
}

bool NvsStorage::eraseAll()
{
    if (!m_open) return false;
    // /nvs ?îÎ†â?∞Î¶¨ ?ÑÏ≤¥ ?åÏùº ??†ú
    File dir = LittleFS.open(kNvsDir, "r");
    if (!dir || !dir.isDirectory()) return false;
    File f = dir.openNextFile();
    while (f) {
        char path[64];
        std::snprintf(path, sizeof(path), "%s/%s", kNvsDir, f.name());
        f.close();
        LittleFS.remove(path);
        f = dir.openNextFile();
    }
    dir.close();
    return true;
}

} // namespace growbed::storage
