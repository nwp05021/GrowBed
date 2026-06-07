#pragma once
#include <cstdint>
#include <cstddef>

namespace growbed::storage
{
    // NVS ?��?구현: LittleFS 기반 ???�일 ?�토리�?
    class NvsStorage
    {
    public:
        static constexpr const char* kNamespace    = "growbed";
        static constexpr const char* kKeySettings  = "settings";
        static constexpr const char* kKeySession   = "session";
        static constexpr const char* kKeyBootCount = "boot_cnt";
        static constexpr const char* kKeyRtState   = "rt_state";
        static constexpr const char* kKeyRtcEpoch  = "rtc_epoch";

        bool init();

        bool saveBlob(const char* key, const void* data, size_t size);
        bool loadBlob(const char* key, void* data, size_t expectedSize);

        bool saveU32(const char* key, uint32_t value);
        bool loadU32(const char* key, uint32_t& outValue);

        bool eraseKey(const char* key);
        bool eraseAll();

        bool isInitialized() const { return m_open; }

    private:
        bool m_open = false;

        void buildPath(char* out, size_t outLen, const char* key) const;
    };
}
