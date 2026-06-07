#include "ui/UiController.h"
#include <algorithm>
#include <cstdio>
#include <ctime>

namespace growbed::ui
{
namespace
{
    int clampInt(int value, int lo, int hi)
    {
        if (value < lo) return lo;
        if (value > hi) return hi;
        return value;
    }

    bool leap(uint16_t y)
    {
        return ((y % 4U) == 0U && (y % 100U) != 0U) || ((y % 400U) == 0U);
    }

    uint8_t maxDayForMonth(uint16_t year, uint8_t month)
    {
        static constexpr uint8_t kDays[] = {31,28,31,30,31,30,31,31,30,31,30,31};
        if (month == 2U) return leap(year) ? 29U : 28U;
        if (month < 1U || month > 12U) return 31U;
        return kDays[month - 1U];
    }

    bool dateFromEpoch(uint32_t epoch, uint16_t& year, uint8_t& month, uint8_t& day)
    {
        if (epoch == 0U) return false;
        std::time_t t = static_cast<std::time_t>(epoch);
        std::tm* tmv = std::localtime(&t);
        if (!tmv) return false;
        year  = static_cast<uint16_t>(tmv->tm_year + 1900);
        month = static_cast<uint8_t>(tmv->tm_mon + 1);
        day   = static_cast<uint8_t>(tmv->tm_mday);
        return year >= 2020U;
    }
}

void UiController::tick(uint32_t nowMs)
{
    if (nowMs - m_lastSyncMs >= kSyncIntervalMs) {
        syncFromState();
        m_lastSyncMs = nowMs;
    }
    if (m_model.screen == UiScreen::FactoryReset) factoryTick(nowMs);
    const bool trayTestSwDown = (m_model.screen == UiScreen::Manual) && m_encoder.isButtonDown();
    m_model.trayLeftTouchTest = trayTestSwDown && (m_model.manualCursor == 2);
    m_model.trayRightTouchTest = trayTestSwDown && (m_model.manualCursor == 3);
    if (m_model.trayLeftTouchTest || m_model.trayRightTouchTest) {
        bool dir = m_model.trayRightTouchTest;
        m_ctrl.applyCommand(app::Cmd::TraySetDirection, &dir, sizeof(dir));
    }
    handleInput();
}

void UiController::syncFromState()
{
    m_model.displayTempC      = m_state.currentTempC;
    m_model.displayHumidPct   = m_state.currentHumidityPct;
    m_model.targetTempC       = m_state.targetTempC;
    m_model.targetHumidPct    = m_state.targetHumidityPct;
    m_model.heaterOn          = m_state.heaterOn;
    m_model.humidifierOn      = m_state.humidifierOn;
    m_model.ledOn             = m_state.ledOn;
    m_model.ledBrightnessPct  = m_state.ledBrightnessPct;
    m_model.ambientLux        = m_state.ambientLux;
    m_model.fanOn             = m_state.fanOn;
    m_model.trayOn            = m_state.trayOn;
    m_model.trayStepHz        = m_state.trayStepHz;
    m_model.trayLeftTouch     = m_state.trayLeftTouch;
    m_model.trayRightTouch    = m_state.trayRightTouch;
    m_model.tempAlarm         = m_state.tempAlarmActive;
    m_model.humiAlarm         = m_state.humiAlarmActive;
    m_model.tempSensorFault   = !m_state.tempSensorOk;
    m_model.humiSensorFault   = !m_state.humiSensorOk;
    m_model.tempSensorWarning = m_state.tempSensorWarning;
    m_model.humiSensorWarning = m_state.humiSensorWarning;
    m_model.sessionActive     = m_state.sessionActive;
    m_model.sessionStartEpoch = m_state.sessionStartEpoch;
    m_model.safeMode          = m_state.safeMode;
    m_model.manualMode        = m_state.manualMode;
    m_model.bootCount         = m_state.bootCount;
    m_model.uptimeMs          = m_state.uptimeMs;
    m_model.rs485Ready        = m_state.rs485Ready;
    m_model.rs485TxActive     = m_state.rs485TxActive;
    m_model.rs485TxCount      = m_state.rs485TxCount;
    m_model.rs485RxCount      = m_state.rs485RxCount;
    m_model.rs485ErrorCount   = m_state.rs485ErrorCount;
    std::snprintf(m_model.rs485Status, sizeof(m_model.rs485Status), "%s", m_state.rs485Status);
    std::snprintf(m_model.rs485LastTx, sizeof(m_model.rs485LastTx), "%s", m_state.rs485LastTx);
    std::snprintf(m_model.rs485LastRx, sizeof(m_model.rs485LastRx), "%s", m_state.rs485LastRx);
}

void UiController::handleInput()
{
    int delta = m_encoder.consumeDelta();
    if (delta != 0) handleDelta(delta);
    if (m_encoder.wasLongPressed()) handleLongPress();
    else if (m_encoder.wasPressed()) handleClick();
}

void UiController::handleDelta(int delta)
{
    const int step = (delta > 0) ? 1 : -1;

    switch (m_model.screen) {
        case UiScreen::Main:
            m_model.activePage = static_cast<uint8_t>(
                (m_model.activePage + Layout::kMainPageCount + step) % Layout::kMainPageCount);
            break;
        case UiScreen::Menu: {
            int cursor = static_cast<int>(m_model.menuCursor) + step;
            if (cursor < 0) cursor = kMainMenuCount - 1;
            if (cursor >= kMainMenuCount) cursor = 0;
            m_model.menuCursor = static_cast<uint8_t>(cursor);
            break;
        }
        case UiScreen::StartDate:
            startDateDelta(step);
            break;
        case UiScreen::Preset:
            presetDelta(step);
            break;
        case UiScreen::RtcSetup:
            rtcSetupDelta(step);
            break;
        case UiScreen::Manual:
            page2Delta(step);
            break;
        case UiScreen::Rs485Test:
            rs485Delta(step);
            break;
        case UiScreen::RebootConfirm:
            m_model.confirmCursor = m_model.confirmCursor ? 0 : 1;
            break;
        default:
            break;
    }
}

void UiController::handleClick()
{
    switch (m_model.screen) {
        case UiScreen::Main:
            goMenu();
            break;
        case UiScreen::Menu:
            enterMenuItem();
            break;
        case UiScreen::StartDate:
            startDateClick();
            break;
        case UiScreen::Preset:
            presetClick();
            break;
        case UiScreen::RtcSetup:
            rtcSetupClick();
            break;
        case UiScreen::Manual:
            page2Click();
            break;
        case UiScreen::Rs485Test:
            rs485Click();
            break;
        case UiScreen::RebootConfirm:
            rebootClick();
            break;
        default:
            break;
    }
}

void UiController::handleLongPress()
{
    switch (m_model.screen) {
        case UiScreen::Main:
            goMenu();
            break;
        case UiScreen::Manual:
            exitManual();
            goMenu();
            break;
        case UiScreen::Rs485Test:
            goMenu();
            break;
        case UiScreen::FactoryReset:
        case UiScreen::Menu:
            goHome();
            break;
        default:
            goMenu();
            break;
    }
}

void UiController::enterMenuItem()
{
    m_model.actionMessage[0] = '\0';
    const UiScreen target = kMainMenuItems[m_model.menuCursor].targetScreen;

    switch (target) {
        case UiScreen::StartDate:
            initDateFromSavedOrNow();
            m_model.fieldCursor = 0;
            m_model.editMode = false;
            m_model.screen = target;
            break;
        case UiScreen::Preset:
            m_model.presetConfirm = false;
            m_model.confirmCursor = 1;
            m_model.screen = target;
            break;
        case UiScreen::RtcSetup:
            initRtcFields();
            m_model.rtcSaveSucceeded = false;
            m_model.fieldCursor = 0;
            m_model.editMode = false;
            m_model.screen = target;
            break;
        case UiScreen::Manual:
            enterManual();
            break;
        case UiScreen::Rs485Test:
            m_model.rs485Cursor = 0;
            m_model.screen = target;
            break;
        case UiScreen::RebootConfirm:
            m_model.confirmCursor = 0;
            m_model.screen = target;
            break;
        case UiScreen::FactoryReset:
            m_model.factoryProgressPct = 0;
            m_model.factoryReady = false;
            m_factoryStartedMs = 0;
            m_model.screen = target;
            break;
        default:
            break;
    }
}

void UiController::enterManual()
{
    m_ctrl.applyCommand(app::Cmd::EnterManualMode);
    m_model.manualCursor = 0;
    m_model.editMode = false;
    m_model.screen = UiScreen::Manual;
}

void UiController::exitManual()
{
    m_ctrl.applyCommand(app::Cmd::ExitManualMode);
}

void UiController::startDateDelta(int d)
{
    if (!m_model.editMode) {
        int c = static_cast<int>(m_model.fieldCursor) + d;
        m_model.fieldCursor = static_cast<uint8_t>(clampInt(c, 0, 4));
        return;
    }

    switch (m_model.fieldCursor) {
        case 0:
            m_model.editBatchYear = static_cast<uint16_t>(
                clampInt(static_cast<int>(m_model.editBatchYear) + d, 2020, 2099));
            break;
        case 1: {
            int m = static_cast<int>(m_model.editBatchMonth) + d;
            if (m < 1) m = 12;
            if (m > 12) m = 1;
            m_model.editBatchMonth = static_cast<uint8_t>(m);
            break;
        }
        case 2:
            m_model.editBatchDay = static_cast<uint8_t>(
                clampInt(static_cast<int>(m_model.editBatchDay) + d,
                         1, maxDayForMonth(m_model.editBatchYear, m_model.editBatchMonth)));
            break;
        default:
            break;
    }

    m_model.editBatchDay = std::min<uint8_t>(
        m_model.editBatchDay,
        maxDayForMonth(m_model.editBatchYear, m_model.editBatchMonth));
}

void UiController::startDateClick()
{
    if (m_model.fieldCursor == 4U) {
        goMenu();
        return;
    }

    if (m_model.fieldCursor <= 2U) {
        m_model.editMode = !m_model.editMode;
        if (!m_model.editMode && m_model.fieldCursor < 3U) ++m_model.fieldCursor;
        return;
    }

    domain::GrowSession sess{};
    sess.species = m_model.selectedSpecies;
    sess.startEpoch = editDateEpoch();

    if (m_ctrl.applyCommand(app::Cmd::StartSession, &sess, sizeof(sess))) {
        m_model.sessionStartEpoch = sess.startEpoch;
        std::snprintf(m_model.actionMessage, sizeof(m_model.actionMessage), "Grow started");
        goMenu();
    } else {
        std::snprintf(m_model.actionMessage, sizeof(m_model.actionMessage), "Start failed");
    }
}

void UiController::presetDelta(int d)
{
    if (m_model.presetConfirm) {
        m_model.confirmCursor = m_model.confirmCursor ? 0 : 1;
        return;
    }

    int c = static_cast<int>(m_model.presetCursor) + d;
    if (c < 0) c = static_cast<int>(domain::kPlantSpeciesCount) - 1;
    if (c >= static_cast<int>(domain::kPlantSpeciesCount)) c = 0;
    m_model.presetCursor = static_cast<uint8_t>(c);
}

void UiController::presetClick()
{
    if (!m_model.presetConfirm) {
        m_model.presetConfirm = true;
        m_model.confirmCursor = 1;
        return;
    }

    if (m_model.confirmCursor == 1U) {
        auto species = static_cast<domain::PlantSpecies>(m_model.presetCursor);
        startSessionFromPreset(species);
    }
    goMenu();
}

void UiController::rtcSetupDelta(int d)
{
    if (!m_model.editMode) {
        const int cursor = static_cast<int>(m_model.fieldCursor) + d;
        m_model.fieldCursor = static_cast<uint8_t>(clampInt(cursor, 0, 7));
        return;
    }

    switch (m_model.fieldCursor) {
        case 0:
            m_model.editRtcYear = static_cast<uint16_t>(
                clampInt(static_cast<int>(m_model.editRtcYear) + d, 2020, 2099));
            break;
        case 1: {
            int month = static_cast<int>(m_model.editRtcMonth) + d;
            if (month < 1) month = 12;
            if (month > 12) month = 1;
            m_model.editRtcMonth = static_cast<uint8_t>(month);
            break;
        }
        case 2:
            m_model.editRtcDay = static_cast<uint8_t>(
                clampInt(static_cast<int>(m_model.editRtcDay) + d, 1,
                         maxDayForMonth(m_model.editRtcYear, m_model.editRtcMonth)));
            break;
        case 3: {
            int hour = static_cast<int>(m_model.editRtcHour) + d;
            if (hour < 0) hour = 23;
            if (hour > 23) hour = 0;
            m_model.editRtcHour = static_cast<uint8_t>(hour);
            break;
        }
        case 4: {
            int minute = static_cast<int>(m_model.editRtcMinute) + d;
            if (minute < 0) minute = 59;
            if (minute > 59) minute = 0;
            m_model.editRtcMinute = static_cast<uint8_t>(minute);
            break;
        }
        case 5: {
            int second = static_cast<int>(m_model.editRtcSecond) + d;
            if (second < 0) second = 59;
            if (second > 59) second = 0;
            m_model.editRtcSecond = static_cast<uint8_t>(second);
            break;
        }
        default:
            break;
    }

    m_model.editRtcDay = std::min<uint8_t>(
        m_model.editRtcDay, maxDayForMonth(m_model.editRtcYear, m_model.editRtcMonth));
}

void UiController::rtcSetupClick()
{
    if (m_model.fieldCursor == 7U) {
        goMenu();
        return;
    }

    if (m_model.fieldCursor <= 5U) {
        if (!m_model.editMode) {
            m_model.actionMessage[0] = '\0';
            m_model.rtcSaveSucceeded = false;
        }
        m_model.editMode = !m_model.editMode;
        if (!m_model.editMode && m_model.fieldCursor < 5U) ++m_model.fieldCursor;
        return;
    }

    std::tm tmv = {};
    tmv.tm_year = static_cast<int>(m_model.editRtcYear) - 1900;
    tmv.tm_mon = static_cast<int>(m_model.editRtcMonth) - 1;
    tmv.tm_mday = static_cast<int>(m_model.editRtcDay);
    tmv.tm_hour = static_cast<int>(m_model.editRtcHour);
    tmv.tm_min = static_cast<int>(m_model.editRtcMinute);
    tmv.tm_sec = static_cast<int>(m_model.editRtcSecond);
    tmv.tm_isdst = -1;

    const std::time_t time = std::mktime(&tmv);
    if (time >= 0 && m_rtc.setEpoch(static_cast<uint32_t>(time))) {
        m_model.rtcSaveSucceeded = true;
        std::snprintf(m_model.actionMessage, sizeof(m_model.actionMessage), "RTC saved");
        goMenu();
    } else {
        m_model.rtcSaveSucceeded = false;
        std::snprintf(m_model.actionMessage, sizeof(m_model.actionMessage), "RTC save failed");
    }
}

void UiController::initRtcFields()
{
    m_model.rtcAvailable = m_rtc.isAvailable();
    const uint32_t epoch = m_rtc.isValid() ? m_rtc.nowEpoch() : 0U;
    const std::time_t time = epoch != 0U ? static_cast<std::time_t>(epoch) : std::time(nullptr);
    std::tm* tmv = std::localtime(&time);
    if (!tmv || tmv->tm_year < 120) return;

    m_model.editRtcYear = static_cast<uint16_t>(tmv->tm_year + 1900);
    m_model.editRtcMonth = static_cast<uint8_t>(tmv->tm_mon + 1);
    m_model.editRtcDay = static_cast<uint8_t>(tmv->tm_mday);
    m_model.editRtcHour = static_cast<uint8_t>(tmv->tm_hour);
    m_model.editRtcMinute = static_cast<uint8_t>(tmv->tm_min);
    m_model.editRtcSecond = static_cast<uint8_t>(tmv->tm_sec);
}

void UiController::rebootClick()
{
    if (m_model.confirmCursor == 1U) {
        m_rtc.persist();
        m_ctrl.applyCommand(app::Cmd::Reboot);
    }
    goMenu();
}

void UiController::factoryTick(uint32_t nowMs)
{
    if (m_encoder.isButtonDown()) {
        if (m_factoryStartedMs == 0U) m_factoryStartedMs = nowMs;
        const uint32_t held = nowMs - m_factoryStartedMs;
        if (held >= 10000U) {
            m_model.factoryProgressPct = 100;
            m_ctrl.applyCommand(app::Cmd::FactoryReset);
            goHome();
        } else {
            m_model.factoryProgressPct = static_cast<uint8_t>((held * 100U) / 10000U);
        }
    } else if (m_factoryStartedMs != 0U) {
        m_factoryStartedMs = 0U;
        m_model.factoryProgressPct = 0U;
    }
}

void UiController::startSessionFromPreset(domain::PlantSpecies species)
{
    m_model.selectedSpecies = species;
    if (m_ctrl.applyCommand(app::Cmd::SelectPlant, &species, sizeof(species))) {
        std::snprintf(m_model.actionMessage, sizeof(m_model.actionMessage),
                      "%s selected", domain::plantName(species));
    }
}

void UiController::goMenu()
{
    m_model.screen = UiScreen::Menu;
    m_model.menuOpen = true;
    m_model.editMode = false;
}

void UiController::goHome()
{
    m_model.screen = UiScreen::Main;
    m_model.menuOpen = false;
    m_model.activePage = 0;
    m_model.editMode = false;
}

void UiController::page2Delta(int d)
{
    if (m_model.manualCursor == 1 && m_model.editMode) {
        const int hz = clampInt(static_cast<int>(m_model.trayStepHz) + (d * 10), 80, 1200);
        uint32_t stepHz = static_cast<uint32_t>(hz);
        if (m_ctrl.applyCommand(app::Cmd::TraySetStepHz, &stepHz, sizeof(stepHz))) {
            m_model.trayStepHz = stepHz;
        }
        return;
    }

    const int cursor = static_cast<int>(m_model.manualCursor) + d;
    m_model.manualCursor = static_cast<int8_t>(clampInt(cursor, 0, 3));
    m_model.editMode = false;
}

void UiController::page2Click()
{
    switch (m_model.manualCursor) {
        case 0:
            m_ctrl.applyCommand(m_model.trayOn ? app::Cmd::TrayOff : app::Cmd::TrayOn);
            break;
        case 1:
            m_model.editMode = !m_model.editMode;
            break;
        default:
            break;
    }
}

void UiController::rs485Delta(int d)
{
    const int cursor = static_cast<int>(m_model.rs485Cursor) + d;
    m_model.rs485Cursor = static_cast<uint8_t>(clampInt(cursor, 0, 2));
}

void UiController::rs485Click()
{
    switch (m_model.rs485Cursor) {
        case 0:
            m_ctrl.applyCommand(app::Cmd::Rs485SendTest);
            break;
        case 1:
            m_ctrl.applyCommand(app::Cmd::Rs485ClearStats);
            break;
        case 2:
            goMenu();
            break;
        default:
            break;
    }
}

uint32_t UiController::editDateEpoch() const
{
    std::tm tmv = {};
    tmv.tm_year = static_cast<int>(m_model.editBatchYear) - 1900;
    tmv.tm_mon = static_cast<int>(m_model.editBatchMonth) - 1;
    tmv.tm_mday = static_cast<int>(m_model.editBatchDay);
    std::time_t t = std::mktime(&tmv);
    return (t < 0) ? 0U : static_cast<uint32_t>(t);
}

void UiController::initDateFromSavedOrNow()
{
    if (dateFromEpoch(m_model.sessionStartEpoch,
                      m_model.editBatchYear,
                      m_model.editBatchMonth,
                      m_model.editBatchDay)) return;

    std::time_t now = std::time(nullptr);
    std::tm* tmv = std::localtime(&now);
    if (tmv && tmv->tm_year >= 120) {
        m_model.editBatchYear = static_cast<uint16_t>(tmv->tm_year + 1900);
        m_model.editBatchMonth = static_cast<uint8_t>(tmv->tm_mon + 1);
        m_model.editBatchDay = static_cast<uint8_t>(tmv->tm_mday);
    }
}

} // namespace growbed::ui
