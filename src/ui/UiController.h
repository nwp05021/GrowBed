#pragma once
#include "ui/UiLayout.h"
#include "ui/UiModel.h"
#include "domain/RuntimeState.h"
#include "domain/PlantSpecies.h"
#include "app/AppController.h"
#include "devices/Ec11Encoder.h"
#include "devices/RtcDriver.h"
#include "infra/ProvisioningManager.h"
#include <cstddef>
#include <cstdint>
#include <ctime>

namespace growbed::ui
{
    class UiController
    {
    public:
        static constexpr uint32_t kSyncIntervalMs = 100;

        UiController(UiModel&                    model,
                     const domain::RuntimeState& state,
                     app::AppController&         ctrl,
                     infra::ProvisioningManager& provisioning,
                     devices::Ec11Encoder&       encoder,
                     devices::RtcDriver&         rtc)
            : m_model(model), m_state(state), m_ctrl(ctrl),
              m_provisioning(provisioning), m_encoder(encoder), m_rtc(rtc)
        {
            m_model.activePage = 0;
        }

        void tick(uint32_t nowMs);

    private:
        void syncFromState();
        void handleInput();
        void handleDelta(int delta);
        void handleClick();
        void handleLongPress();
        void page2Delta(int d);
        void page2Click();
        void rs485Delta(int d);
        void rs485Click();
        void enterMenuItem();
        void enterManual();
        void exitManual();
        void startDateDelta(int d);
        void startDateClick();
        void presetDelta(int d);
        void presetClick();
        void rtcSetupDelta(int d);
        void rtcSetupClick();
        void initRtcFields();
        void rebootClick();
        void factoryTick(uint32_t nowMs);
        void startSessionFromPreset(domain::PlantSpecies species);
        void goMenu();
        void goHome();
        uint32_t editDateEpoch() const;
        void initDateFromSavedOrNow();

        UiModel&                    m_model;
        const domain::RuntimeState& m_state;
        app::AppController&         m_ctrl;
        infra::ProvisioningManager& m_provisioning;
        devices::Ec11Encoder&       m_encoder;
        devices::RtcDriver&         m_rtc;

        uint32_t m_lastSyncMs       = 0;
        uint32_t m_factoryStartedMs = 0;
    };
}
