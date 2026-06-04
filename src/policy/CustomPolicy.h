#pragma once
#include "policy/IPlantPolicy.h"

namespace growbed::policy
{
    class CustomPolicy final : public IPlantPolicy
    {
    public:
        struct Params
        {
            float    targetTempC        = 22.0f;
            float    targetHumidityPct  = 60.0f;
            uint8_t  lightOnHour        = 6U;
            uint8_t  lightOffHour       = 20U;
            uint8_t  lightBrightnessPct = 75U;
            uint16_t ambientDimLux      = 3000U;
        };

        CustomPolicy() : m_p(Params{}) {}
        explicit CustomPolicy(const Params& p) : m_p(p) {}

        const char* name()               const override { return "Custom"; }
        float       targetTempC()        const override { return m_p.targetTempC; }
        float       targetHumidityPct()  const override { return m_p.targetHumidityPct; }
        uint8_t     lightOnHour()        const override { return m_p.lightOnHour; }
        uint8_t     lightOffHour()       const override { return m_p.lightOffHour; }
        uint8_t     lightBrightnessPct() const override { return m_p.lightBrightnessPct; }
        uint16_t    ambientDimLux()      const override { return m_p.ambientDimLux; }

    private:
        Params m_p;
    };
}
