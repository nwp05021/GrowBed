#pragma once
#include "domain/PlantSpecies.h"
#include <cstdint>

namespace growbed::ui
{
    enum class EditField : uint8_t { None, Temp, Humidity };

    enum class UiScreen : uint8_t
    {
        Main,
        Menu,
        StartDate,
        Preset,
        RtcSetup,
        Manual,
        Rs485Test,
        RebootConfirm,
        FactoryReset,
        System,
    };

    struct MenuItem {
        const char* name;
        const char* desc;
        UiScreen    targetScreen;
    };

    static constexpr MenuItem kMainMenuItems[] = {
        { "Start Grow",    "Enter date",   UiScreen::StartDate     },
        { "Plant Policy",  "Select target", UiScreen::Preset        },
        { "RTC Setup",     "Date / time",  UiScreen::RtcSetup      },
        { "Grow Tray Test", "Tray",         UiScreen::Manual        },
        { "RS485 Test",    "PING test",     UiScreen::Rs485Test     },
        { "Reboot",        "System",        UiScreen::RebootConfirm },
        { "Factory Reset", "10 sec hold",   UiScreen::FactoryReset  },
    };

    static constexpr uint8_t kMainMenuCount =
        sizeof(kMainMenuItems) / sizeof(kMainMenuItems[0]);

    struct UiModel
    {
        float    displayTempC       = 0.0f;
        float    displayHumidPct    = 0.0f;
        float    targetTempC        = 22.0f;
        float    targetHumidPct     = 65.0f;
        bool     heaterOn           = false;
        bool     humidifierOn       = false;
        bool     ledOn              = false;
        uint8_t  ledBrightnessPct   = 0;
        uint16_t ambientLux         = 0;
        bool     fanOn              = false;
        bool     trayOn             = false;
        uint32_t trayStepHz         = 800;
        bool     trayLeftTouch      = false;
        bool     trayRightTouch     = false;
        bool     trayLeftTouchTest  = false;
        bool     trayRightTouchTest = false;
        bool     tempAlarm          = false;
        bool     humiAlarm          = false;
        bool     tempSensorFault    = false;
        bool     humiSensorFault    = false;
        bool     tempSensorWarning  = false;
        bool     humiSensorWarning  = false;

        bool     sessionActive      = false;
        uint32_t sessionStartEpoch  = 0;
        domain::PlantSpecies selectedSpecies = domain::PlantSpecies::Lettuce;

        bool     safeMode           = false;
        bool     manualMode         = false;
        uint32_t bootCount          = 0;
        uint32_t uptimeMs           = 0;
        char     fwVersion[12]      = "v1.0.0";

        UiScreen screen             = UiScreen::Main;
        uint8_t  activePage         = 0;
        bool     menuOpen           = false;
        uint8_t  menuCursor         = 0;
        int8_t   manualCursor       = 0;
        uint8_t  rs485Cursor        = 0;
        uint8_t  confirmCursor      = 0;
        uint8_t  presetCursor       = 0;
        bool     presetConfirm      = false;
        bool     editMode           = false;
        uint8_t  fieldCursor        = 0;
        EditField activeEditField   = EditField::None;
        uint8_t  factoryProgressPct = 0;
        bool     factoryReady       = false;
        char     actionMessage[40]  = {};

        uint16_t editBatchYear      = 2026;
        uint8_t  editBatchMonth     = 1;
        uint8_t  editBatchDay       = 1;

        bool     rtcAvailable       = false;
        bool     rtcSaveSucceeded   = false;
        uint16_t editRtcYear        = 2026;
        uint8_t  editRtcMonth       = 1;
        uint8_t  editRtcDay         = 1;
        uint8_t  editRtcHour        = 0;
        uint8_t  editRtcMinute      = 0;
        uint8_t  editRtcSecond      = 0;

        bool     rs485Ready         = false;
        bool     rs485TxActive      = false;
        uint32_t rs485TxCount       = 0;
        uint32_t rs485RxCount       = 0;
        uint32_t rs485ErrorCount    = 0;
        char     rs485Status[24]    = {};
        char     rs485LastTx[32]    = {};
        char     rs485LastRx[48]    = {};

        bool     cloudConnected     = false;
        bool     wifiConnected      = false;
        char     ipAddress[16]      = {};

        bool     provisioningActive      = false;
        bool     provisioningSucceeded   = false;
        bool     provisioningFailed      = false;
        uint32_t provisioningRemainingMs = 0;
        char     provisioningName[32]    = {};
        char     provisioningPop[16]     = {};
        char     provisioningMessage[40] = {};
    };
}
