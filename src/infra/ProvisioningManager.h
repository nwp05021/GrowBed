#pragma once
#include <cstdint>

// RP2040 ?¬íŠ¸: ProvisioningManager ?¤í…
// WiFi/BLE ?„ë¡œë¹„ì??ì´ ?†ìœ¼ë¯€ë¡?ëª¨ë“  ?íƒœë¥?ë¹„í™œ?±ìœ¼ë¡?ë°˜í™˜?©ë‹ˆ??
namespace growbed::infra
{
    class ProvisioningManager
    {
    public:
        ProvisioningManager() = default;

        void init()                                {}
        void startBootProvisioning(uint32_t)       {}
        void requestMenuProvisioning(uint32_t)     {}
        void cancel()                              {}
        void tick(uint32_t)                        {}

        bool isActive()     const { return false; }
        bool isConnected()  const { return false; }
        bool isConfigured() const { return false; }
        bool isLocalMode()  const { return false; }
        bool isSucceeded()  const { return false; }
        bool isAdvertising() const { return false; }
        bool isFailed()     const { return false; }

        uint32_t remainingMs(uint32_t) const { return 0U; }

        const char* deviceName()        const { return ""; }
        const char* proofOfPossession() const { return ""; }
        const char* statusText()        const { return ""; }
    };
}
