#pragma once
#include <cstdint>

namespace growbed::domain
{
    struct RuntimeState
    {
        float    currentTempC        = 0.0f;
        float    currentHumidityPct  = 0.0f;
        float    targetTempC         = 22.0f;
        float    targetHumidityPct   = 65.0f;
        bool     tempSensorOk        = false;
        bool     humiSensorOk        = false;
        bool     tempSensorWarning   = false;
        bool     humiSensorWarning   = false;

        bool     heaterOn            = false;
        bool     humidifierOn        = false;
        bool     fanOn               = false;
        bool     trayOn              = false;
        uint32_t trayStepHz          = 800;
        bool     trayLeftTouch       = false;
        bool     trayRightTouch      = false;
        bool     ledOn               = false;
        uint8_t  ledBrightnessPct    = 0;

        uint16_t ambientLux          = 0;
        bool     lightSensorOk       = false;

        bool     tempAlarmActive     = false;
        bool     humiAlarmActive     = false;

        bool     safeMode            = false;
        bool     sessionActive       = false;
        bool     manualMode          = false;

        bool     cloudConnected      = false;
        bool     wifiConnected       = false;
        char     ipAddress[16]       = {};

        uint32_t uptimeMs            = 0;
        uint32_t bootCount           = 0;
        uint32_t sessionStartEpoch   = 0;

        bool     rs485Ready          = false;
        bool     rs485TxActive       = false;
        uint32_t rs485TxCount        = 0;
        uint32_t rs485RxCount        = 0;
        uint32_t rs485ErrorCount     = 0;
        uint32_t rs485TestRequestSeq = 0;
        uint32_t rs485LastTestMs     = 0;
        char     rs485Status[24]     = "IDLE";
        char     rs485LastTx[32]     = {};
        char     rs485LastRx[48]     = {};

        static RuntimeState zero()
        {
            RuntimeState s{};
            return s;
        }
    };
}
