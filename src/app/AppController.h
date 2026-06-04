#pragma once
#include "domain/AppSettings.h"
#include "domain/RuntimeState.h"
#include "domain/GrowSession.h"
#include "domain/PlantSpecies.h"
#include "devices/GpioOutput.h"
#include "devices/PwmFan.h"
#include "modules/GrowLightModule.h"
#include "modules/GrowTrayModule.h"
#include "storage/NvsStorage.h"
#include "policy/IPlantPolicy.h"
#include "policy/PolicyFactory.h"
#include <cstdint>
#include <cstddef>
#include <memory>

namespace growbed::app
{
    enum class Cmd : uint16_t
    {
        StartSession     = 100,
        StopSession      = 101,
        SelectPlant      = 102,

        HeaterOn         = 300,
        HeaterOff        = 301,
        HumidOn          = 302,
        HumidOff         = 303,
        LedOn            = 304,
        LedOff           = 305,
        FanSetDuty       = 306,
        LedSetBrightness = 307,
        TrayOn           = 308,
        TrayOff          = 309,
        Rs485SendTest    = 310,
        Rs485ClearStats  = 311,
        TraySetStepHz    = 312,
        TraySetDirection = 313,

        UpdateSettings   = 400,

        ClearSafeMode    = 500,
        FactoryReset     = 501,

        EnterManualMode  = 600,
        ExitManualMode   = 601,

        ClearWifiInfo    = 700,

        Reboot           = 800,
    };

    class AppController
    {
    public:
        AppController(domain::RuntimeState&  state,
                      domain::AppSettings&   settings,
                      domain::GrowSession&   session,
                      storage::NvsStorage&   nvs,
                      devices::GpioOutput&   heater,
                      devices::GpioOutput&   humidifier,
                      modules::GrowLightModule& growLight,
                      modules::GrowTrayModule&  growTray,
                      devices::PwmFan&       fan)
            : m_state(state), m_settings(settings), m_session(session),
              m_nvs(nvs),
              m_heater(heater), m_humidifier(humidifier),
              m_growLight(growLight), m_growTray(growTray), m_fan(fan) {}

        bool applyCommand(Cmd cmd,
                          const void* payload = nullptr,
                          size_t      payloadSz = 0);

        bool restoreFromStorage();
        const policy::IPlantPolicy* currentPolicy() const { return m_policy.get(); }

    private:
        domain::RuntimeState& m_state;
        domain::AppSettings&  m_settings;
        domain::GrowSession&  m_session;
        storage::NvsStorage&  m_nvs;
        devices::GpioOutput&  m_heater;
        devices::GpioOutput&  m_humidifier;
        modules::GrowLightModule& m_growLight;
        modules::GrowTrayModule&  m_growTray;
        devices::PwmFan&      m_fan;

        std::unique_ptr<policy::IPlantPolicy> m_policy;

        bool cmdStartSession(const domain::GrowSession& s);
        bool cmdStopSession();
        bool cmdSelectPlant(domain::PlantSpecies species);
        bool cmdUpdateSettings(const domain::AppSettings& s);
        bool cmdFactoryReset();
        bool cmdClearWifiInfo();
        bool cmdEnterManualMode();
        bool cmdExitManualMode();
        bool cmdClearSafeMode();

        void generateSessionId(domain::GrowSession& s);
        void applySessionToState();
        void rebuildPolicy();
    };
}
