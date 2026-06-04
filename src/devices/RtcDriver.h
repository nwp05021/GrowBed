#pragma once
#include <RTClib.h>
#include <cstdint>

// DS3231 RTC ?œë¼?´ë²„.
// I2cBus::init() ?´í›„??init() ë¥??¸ì¶œ?´ì•¼ ?©ë‹ˆ??
// DS3231 I2C ì£¼ì†Œ 0x68 ??AHT20(0x38)ê³?ë²„ìŠ¤ ê³µìœ  ê°€??
namespace growbed::devices
{
    class RtcDriver
    {
    public:
        // Wire(I2C) ì´ˆê¸°?????¸ì¶œ. ?”ë°”?´ìŠ¤ê°€ ?†ìœ¼ë©?false ë°˜í™˜.
        bool init();

        // RTC ì¹©ì´ ë°œê²¬?˜ê³  ? íš¨???œê°„??ë³´ìœ  ì¤‘ì¸ì§€ ?•ì¸.
        // 2020-01-01 ?´ì „ ê°’ì´ë©?? íš¨?˜ì? ?Šì? ê²ƒìœ¼ë¡??ë‹¨.
        bool isValid() const;

        // ?„ì¬ Unix timestamp (UTC) ë°˜í™˜. ? íš¨?˜ì? ?Šìœ¼ë©?0.
        uint32_t nowEpoch() const;

        // RTC???œê°„???°ê³ , ?œìŠ¤???œê°„???™ê¸°??
        bool setEpoch(uint32_t epoch);

        // RTC ??settimeofday() ë¡??œìŠ¤???œê°„ ?™ê¸°??(KST ?€?„ì¡´ ?¬í•¨).
        bool syncSystemTime() const;

        bool isAvailable() const { return m_available; }

    private:
        mutable RTC_DS3231 m_rtc;
        bool       m_available = false;

        static constexpr uint32_t kMinValidEpoch = 1577836800U; // 2020-01-01 UTC
    };
}
