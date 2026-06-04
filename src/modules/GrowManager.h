#pragma once

#include "modules/GrowLightModule.h"
#include "modules/GrowTrayModule.h"
#include <cstdint>

namespace growbed::modules
{
    class GrowManager
    {
    public:
        struct Config
        {
            uint8_t  startHour = 6U;
            uint8_t  startMinute = 0U;
            uint8_t  endHour = 20U;
            uint8_t  endMinute = 0U;
            uint32_t pollIntervalMs = 1000U;
        };

        GrowManager(GrowLightModule& lightModule,
                    GrowTrayModule& trayModule)
            : GrowManager(lightModule, trayModule, Config{}) {}

        GrowManager(GrowLightModule& lightModule,
                    GrowTrayModule& trayModule,
                    const Config& config)
            : m_lightModule(lightModule),
              m_trayModule(trayModule),
              m_config(config) {}

        bool init();
        void tick(uint32_t nowMs);

        bool isActive() const { return m_active; }

    private:
        GrowLightModule& m_lightModule;
        GrowTrayModule&  m_trayModule;
        Config           m_config;

        bool     m_active = false;
        uint32_t m_lastPollMs = 0U;

        bool scheduleAllowsGrow() const;
        void applyActive(bool active);
    };
}
