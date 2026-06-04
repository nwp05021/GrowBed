#pragma once
#include "policy/IPlantPolicy.h"

namespace growbed::policy
{
    // 허브 재배
    // 온도 22°C, 습도 57%, 광주기 14h (06:00~20:00), 밝기 70%.
    class HerbPolicy final : public IPlantPolicy
    {
    public:
        const char* name()               const override { return "허브"; }
        float       targetTempC()        const override { return 22.0f; }
        float       targetHumidityPct()  const override { return 57.0f; }
        uint8_t     lightOnHour()        const override { return 6U;  }
        uint8_t     lightOffHour()       const override { return 20U; }
        uint8_t     lightBrightnessPct() const override { return 70U; }
        uint16_t    ambientDimLux()      const override { return 3000U; }
    };
}
