#pragma once
#include "policy/IPlantPolicy.h"

namespace growbed::policy
{
    // 고추 재배
    // 온도 20°C, 습도 65%, 광주기 14h (06:00~20:00), 밝기 80%.
    class LettucePolicy final : public IPlantPolicy
    {
    public:
        const char* name()               const override { return "고추"; }
        float       targetTempC()        const override { return 20.0f; }
        float       targetHumidityPct()  const override { return 65.0f; }
        uint8_t     lightOnHour()        const override { return 6U;  }
        uint8_t     lightOffHour()       const override { return 20U; }
        uint8_t     lightBrightnessPct() const override { return 80U; }
        uint16_t    ambientDimLux()      const override { return 3000U; }
    };
}
