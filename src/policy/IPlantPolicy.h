#pragma once
#include <cstdint>

namespace growbed::policy
{
    class IPlantPolicy
    {
    public:
        virtual ~IPlantPolicy() = default;

        virtual const char* name()               const = 0;
        virtual float       targetTempC()        const = 0;
        virtual float       targetHumidityPct()  const = 0;
        virtual uint8_t     lightOnHour()        const = 0;
        virtual uint8_t     lightOffHour()       const = 0;
        virtual uint8_t     lightBrightnessPct() const = 0;
        virtual uint16_t    ambientDimLux()      const = 0;
    };
}
