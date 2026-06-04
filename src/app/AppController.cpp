#include "app/AppController.h"
#include <Arduino.h>
#include <cstdio>
#include <cstring>
#include <ctime>

namespace growbed::app
{

bool AppController::applyCommand(Cmd cmd, const void* payload, size_t sz)
{
    switch (cmd)
    {
    case Cmd::StartSession:
        if (!payload || sz != sizeof(domain::GrowSession)) return false;
        return cmdStartSession(*static_cast<const domain::GrowSession*>(payload));

    case Cmd::StopSession:
        return cmdStopSession();

    case Cmd::SelectPlant:
        if (!payload || sz != sizeof(domain::PlantSpecies)) return false;
        return cmdSelectPlant(*static_cast<const domain::PlantSpecies*>(payload));

    case Cmd::HeaterOn:
        m_state.safeMode = false;
        m_heater.on();
        m_state.heaterOn = true;
        return true;

    case Cmd::HeaterOff:
        m_heater.off();
        m_state.heaterOn = false;
        return true;

    case Cmd::HumidOn:
        m_humidifier.on();
        m_state.humidifierOn = true;
        return true;

    case Cmd::HumidOff:
        m_humidifier.off();
        m_state.humidifierOn = false;
        return true;

    case Cmd::LedOn:
        m_growLight.on();
        m_state.ledOn            = true;
        return true;

    case Cmd::LedOff:
        m_growLight.off();
        m_state.ledOn            = false;
        m_state.ledBrightnessPct = 0U;
        return true;

    case Cmd::LedSetBrightness:
        if (!payload || sz != sizeof(uint8_t)) return false;
        {
            uint8_t pct = *static_cast<const uint8_t*>(payload);
            if (pct > 100U) pct = 100U;
            m_growLight.setOn(pct > 0U);
            m_state.ledOn            = (pct > 0U);
            m_state.ledBrightnessPct = pct;
        }
        return true;

    case Cmd::FanSetDuty:
        if (!payload || sz != sizeof(uint8_t)) return false;
        {
            uint8_t duty = *static_cast<const uint8_t*>(payload);
            m_fan.setDuty(duty);
            m_state.fanOn = (duty > 0U);
        }
        return true;

    case Cmd::TrayOn:
        m_growTray.on();
        m_state.trayOn = true;
        return true;

    case Cmd::TrayOff:
        m_growTray.off();
        m_state.trayOn = false;
        return true;

    case Cmd::TraySetStepHz:
        if (!payload || sz != sizeof(uint32_t)) return false;
        m_growTray.setStepHz(*static_cast<const uint32_t*>(payload));
        m_state.trayStepHz = m_growTray.stepHz();
        return true;

    case Cmd::TraySetDirection:
        if (!payload || sz != sizeof(bool)) return false;
        m_growTray.setCurrentDir(*static_cast<const bool*>(payload));
        return true;

    case Cmd::Rs485SendTest:
        ++m_state.rs485TestRequestSeq;
        return true;

    case Cmd::Rs485ClearStats:
        m_state.rs485TxCount = 0;
        m_state.rs485RxCount = 0;
        m_state.rs485ErrorCount = 0;
        m_state.rs485LastTx[0] = '\0';
        m_state.rs485LastRx[0] = '\0';
        std::snprintf(m_state.rs485Status, sizeof(m_state.rs485Status), "CLEARED");
        return true;

    case Cmd::UpdateSettings:
        if (!payload || sz != sizeof(domain::AppSettings)) return false;
        return cmdUpdateSettings(*static_cast<const domain::AppSettings*>(payload));

    case Cmd::ClearSafeMode:
        return cmdClearSafeMode();

    case Cmd::FactoryReset:
        return cmdFactoryReset();

    case Cmd::EnterManualMode:
        return cmdEnterManualMode();

    case Cmd::ExitManualMode:
        return cmdExitManualMode();

    case Cmd::ClearWifiInfo:
        return cmdClearWifiInfo();

    case Cmd::Reboot:
        Serial.println("[AppCtrl] Reboot requested");
        rp2040.reboot();
        return true;

    default:
        Serial.printf("[AppCtrl] Unknown Cmd: %u\n", static_cast<unsigned>(cmd));
        return false;
    }
}

bool AppController::cmdStartSession(const domain::GrowSession& s)
{
    m_session        = s;
    m_session.active = true;
    if (m_session.sessionId[0] == '\0') generateSessionId(m_session);
    if (m_session.startEpoch == 0U) {
        m_session.startEpoch = static_cast<uint32_t>(std::time(nullptr));
    }
    rebuildPolicy();
    applySessionToState();
    if (!m_nvs.saveBlob(storage::NvsStorage::kKeySession, &m_session, sizeof(m_session))) {
        Serial.println("[AppCtrl] Session save failed");
        m_state.safeMode = true;
        return false;
    }
    Serial.printf("[AppCtrl] Session started: %s (%s)\n",
                  m_session.sessionId, domain::plantName(m_session.species));
    return true;
}

bool AppController::cmdStopSession()
{
    m_session.active      = false;
    m_state.sessionActive = false;
    m_nvs.saveBlob(storage::NvsStorage::kKeySession, &m_session, sizeof(m_session));
    Serial.println("[AppCtrl] Session stopped");
    return true;
}

bool AppController::cmdSelectPlant(domain::PlantSpecies species)
{
    m_session.species = species;
    rebuildPolicy();
    if (m_policy) {
        m_state.targetTempC       = m_policy->targetTempC();
        m_state.targetHumidityPct = m_policy->targetHumidityPct();
    }
    m_nvs.saveBlob(storage::NvsStorage::kKeySession, &m_session, sizeof(m_session));
    Serial.printf("[AppCtrl] Plant selected: %s\n", domain::plantName(species));
    return true;
}

bool AppController::cmdUpdateSettings(const domain::AppSettings& s)
{
    if (!s.isValid()) return false;
    m_settings = s;
    return m_nvs.saveBlob(storage::NvsStorage::kKeySettings, &m_settings, sizeof(m_settings));
}

bool AppController::cmdFactoryReset()
{
    m_nvs.eraseAll();
    m_settings = domain::AppSettings::defaults();
    m_session  = {};
    m_state    = domain::RuntimeState::zero();
    m_policy.reset();
    Serial.println("[AppCtrl] Factory reset complete");
    return true;
}

bool AppController::cmdClearSafeMode()
{
    m_state.safeMode = false;
    return true;
}

bool AppController::cmdClearWifiInfo()
{
    m_settings.wifiConfigured = false;
    std::memset(m_settings.wifiSsid,     0, sizeof(m_settings.wifiSsid));
    std::memset(m_settings.wifiPassword, 0, sizeof(m_settings.wifiPassword));
    m_nvs.saveBlob(storage::NvsStorage::kKeySettings, &m_settings, sizeof(m_settings));
    return true;
}

bool AppController::cmdEnterManualMode()
{
    m_state.manualMode = true;
    m_state.safeMode   = false;
    m_heater.off();     m_state.heaterOn     = false;
    m_humidifier.off(); m_state.humidifierOn = false;
    m_growLight.off();  m_state.ledOn        = false;
    m_fan.stop();       m_state.fanOn        = false;
    m_growTray.off();   m_state.trayOn       = false;
    return true;
}

bool AppController::cmdExitManualMode()
{
    m_heater.off();     m_state.heaterOn     = false;
    m_humidifier.off(); m_state.humidifierOn = false;
    m_growLight.off();  m_state.ledOn        = false;
    m_fan.stop();       m_state.fanOn        = false;
    m_growTray.off();   m_state.trayOn       = false;
    m_state.manualMode = false;
    return true;
}

bool AppController::restoreFromStorage()
{
    domain::AppSettings loaded{};
    if (m_nvs.loadBlob(storage::NvsStorage::kKeySettings, &loaded, sizeof(loaded)) && loaded.isValid()) {
        m_settings = loaded;
    } else {
        m_settings = domain::AppSettings::defaults();
        m_nvs.saveBlob(storage::NvsStorage::kKeySettings, &m_settings, sizeof(m_settings));
    }

    uint32_t cnt = 0;
    m_nvs.loadU32(storage::NvsStorage::kKeyBootCount, cnt);
    cnt++;
    m_nvs.saveU32(storage::NvsStorage::kKeyBootCount, cnt);
    m_state.bootCount = cnt;
    Serial.printf("[AppCtrl] Boot count: %u\n", static_cast<unsigned>(cnt));

    domain::GrowSession sess{};
    if (m_nvs.loadBlob(storage::NvsStorage::kKeySession, &sess, sizeof(sess)) && sess.isValid()) {
        m_session = sess;
        rebuildPolicy();
        applySessionToState();
        Serial.printf("[AppCtrl] Session restored: %s (%s)\n",
                      m_session.sessionId, domain::plantName(m_session.species));
    }
    return true;
}

void AppController::rebuildPolicy()
{
    m_policy = policy::PolicyFactory::create(m_session.species);
    if (m_policy) {
        m_state.targetTempC       = m_policy->targetTempC();
        m_state.targetHumidityPct = m_policy->targetHumidityPct();
    }
}

void AppController::generateSessionId(domain::GrowSession& s)
{
    std::snprintf(s.sessionId, sizeof(s.sessionId), "GRW-%03u",
                  static_cast<unsigned>(m_state.bootCount % 1000U));
}

void AppController::applySessionToState()
{
    m_state.sessionActive    = m_session.active;
    m_state.sessionStartEpoch = m_session.startEpoch;
}

} // namespace growbed::app
