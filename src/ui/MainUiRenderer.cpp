#include "ui/MainUiRenderer.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>

namespace growbed::ui
{
namespace
{
    void hashBytes(uint32_t& hash, const void* data, size_t len)
    {
        const auto* p = static_cast<const uint8_t*>(data);
        while (len-- > 0U) {
            hash ^= *p++;
            hash *= 16777619U;
        }
    }

    template <typename T>
    void hashValue(uint32_t& hash, const T& value)
    {
        hashBytes(hash, &value, sizeof(T));
    }

    void hashString(uint32_t& hash, const char* text)
    {
        if (!text) {
            uint8_t z = 0;
            hashValue(hash, z);
            return;
        }
        while (*text) {
            uint8_t c = static_cast<uint8_t>(*text++);
            hashValue(hash, c);
        }
        uint8_t z = 0;
        hashValue(hash, z);
    }

    int16_t q10(float v) { return static_cast<int16_t>(std::lroundf(v * 10.0f)); }
    int16_t q1(float v)  { return static_cast<int16_t>(std::lroundf(v)); }

    bool hasUtf8Text(const char* text)
    {
        if (!text) return false;
        while (*text) {
            if ((static_cast<unsigned char>(*text) & 0x80U) != 0U) return true;
            ++text;
        }
        return false;
    }

    const char* screenTitle(const UiModel& model)
    {
        switch (model.screen) {
            case UiScreen::Menu:          return "Menu";
            case UiScreen::StartDate:     return "Start Grow";
            case UiScreen::Preset:        return "Plant Policy";
            case UiScreen::Manual:        return "Grow Tray Test";
            case UiScreen::Rs485Test:     return "RS485 Test";
            case UiScreen::System:        return "System";
            case UiScreen::RebootConfirm: return "Reboot";
            case UiScreen::FactoryReset:  return "Factory Reset";
            default: break;
        }
        switch (model.activePage) {
            case 1:  return "Status";
            case 2:  return "System";
            default: return "GrowBed";
        }
    }

    void formatDate(char* out, size_t len)
    {
        std::time_t now = std::time(nullptr);
        std::tm* tmv = std::localtime(&now);
        if (tmv && tmv->tm_year >= 120) {
            std::snprintf(out, len, "%04d-%02d-%02d",
                          tmv->tm_year + 1900, tmv->tm_mon + 1, tmv->tm_mday);
        } else {
            std::snprintf(out, len, "---- -- --");
        }
    }

    uint32_t getHeaderHash(const UiModel& model, uint32_t nowMs)
    {
        uint32_t h = 2166136261U;
        hashValue(h, model.screen);
        hashValue(h, model.activePage);
        if (model.screen == UiScreen::Main && model.activePage == 0U) {
            hashValue(h, nowMs / 500U);
        }
        if (model.screen == UiScreen::Main) {
            hashValue(h, model.uptimeMs / 1000U);
        }
        hashValue(h, model.tempAlarm);
        hashValue(h, model.humiAlarm);
        hashValue(h, model.tempSensorFault);
        hashValue(h, model.humiSensorFault);
        return h;
    }

    uint32_t getFooterHash(const UiModel& model, uint32_t)
    {
        uint32_t h = 2166136261U;
        hashValue(h, model.screen);
        hashValue(h, model.sessionActive);
        hashValue(h, model.heaterOn);
        hashValue(h, model.humidifierOn);
        hashValue(h, model.ledOn);
        hashValue(h, model.trayOn);
        hashValue(h, model.fanOn);
        return h;
    }

    uint32_t getPageHash(const UiModel& model, uint32_t nowMs)
    {
        uint32_t h = 2166136261U;
        hashValue(h, model.screen);
        hashValue(h, model.activePage);
        hashValue(h, model.sessionActive);
        hashValue(h, model.safeMode);
        hashValue(h, q10(model.displayTempC));
        hashValue(h, q1(model.displayHumidPct));
        hashValue(h, q10(model.targetTempC));
        hashValue(h, q1(model.targetHumidPct));
        hashValue(h, model.ledOn);
        hashValue(h, model.ledBrightnessPct);
        hashValue(h, model.ambientLux);
        hashValue(h, model.menuCursor);
        hashValue(h, model.manualCursor);
        hashValue(h, model.rs485Cursor);
        hashValue(h, model.heaterOn);
        hashValue(h, model.humidifierOn);
        hashValue(h, model.fanOn);
        hashValue(h, model.trayOn);
        hashValue(h, model.trayStepHz);
        hashValue(h, model.trayLeftTouch);
        hashValue(h, model.trayRightTouch);
        hashValue(h, model.trayLeftTouchTest);
        hashValue(h, model.trayRightTouchTest);
        hashValue(h, model.rs485Ready);
        hashValue(h, model.rs485TxActive);
        hashValue(h, model.rs485TxCount);
        hashValue(h, model.rs485RxCount);
        hashValue(h, model.rs485ErrorCount);
        hashString(h, model.rs485Status);
        hashString(h, model.rs485LastTx);
        hashString(h, model.rs485LastRx);
        hashValue(h, model.confirmCursor);
        hashValue(h, model.presetCursor);
        hashValue(h, model.presetConfirm);
        hashValue(h, model.editMode);
        hashValue(h, model.fieldCursor);
        hashValue(h, model.editBatchYear);
        hashValue(h, model.editBatchMonth);
        hashValue(h, model.editBatchDay);
        hashValue(h, model.factoryProgressPct);
        hashValue(h, model.factoryReady);
        hashString(h, model.actionMessage);
        hashValue(h, model.bootCount);
        if (model.screen == UiScreen::Main) {
            hashValue(h, model.uptimeMs / 1000U);
            hashValue(h, nowMs / 5000U);
        }
        return h;
    }
}

void MainUiRenderer::render(uint32_t nowMs)
{
    if (m_hasRendered && nowMs - m_lastRenderMs < 30U) return;

    const uint32_t curHeaderHash = getHeaderHash(m_model, nowMs);
    const uint32_t curPageHash = getPageHash(m_model, nowMs);
    const uint32_t curFooterHash = getFooterHash(m_model, nowMs);

    bool headerDirty = m_firstRender || (curHeaderHash != m_lastHeaderHash);
    bool pageDirty = m_firstRender || (curPageHash != m_lastPageHash);
    bool footerDirty = m_firstRender || (curFooterHash != m_lastFooterHash);

    if (!headerDirty && !pageDirty && !footerDirty) return;

    m_renderNowMs = nowMs;
    m_lastRenderMs = nowMs;

    m_display.beginFrame();

    if (m_model.provisioningActive) {
        if (!m_wasProvisioning) {
            m_provisioningRenderer.reset();
            m_wasProvisioning = true;
        }
        m_provisioningRenderer.render(nowMs);
        m_hasRendered = true;
        m_display.endFrame();
        return;
    }

    if (m_wasProvisioning) {
        m_wasProvisioning = false;
        m_firstRender = true;
    }

    m_lastHeaderHash = curHeaderHash;
    m_lastPageHash = curPageHash;
    m_lastFooterHash = curFooterHash;
    m_firstRender = false;
    m_hasRendered = true;

    if (headerDirty) drawStatusBar(nowMs);

    if (pageDirty) {
        m_display.fillRect(0, 26, Layout::kScreenW, 189, Color::kBg);

        switch (m_model.screen) {
            case UiScreen::Main:
                if (m_model.activePage == 0) m_pageMain.render(nowMs);
                else if (m_model.activePage == 1) renderPage1();
                else m_pageHelp.render(nowMs);
                break;
            case UiScreen::Menu:          m_pageMenu.render(nowMs);       break;
            case UiScreen::StartDate:     m_pageStartDate.render(nowMs);  break;
            case UiScreen::Preset:        m_pagePreset.render(nowMs);     break;
            case UiScreen::Manual:        m_pageGrowTrayTest.render(nowMs); break;
            case UiScreen::Rs485Test:     m_pageRs485Test.render(nowMs);  break;
            case UiScreen::RebootConfirm:
                renderConfirm("Reboot system",
                              "Restart the controller.",
                              "Outputs may pause briefly.");
                break;
            case UiScreen::FactoryReset:  m_pageFactoryReset.render(nowMs); break;
            default: break;
        }

        if (m_model.safeMode && m_model.screen == UiScreen::Main) {
            m_display.fillRect(72, 102, 176, 34, Color::kDanger);
            m_display.setTextSize(2);
            m_display.setTextColor(Color::kText, Color::kDanger);
            m_display.drawText(104, 112, "SAFE MODE");
        }
    }

    if (footerDirty) renderFooter(nowMs);

    m_display.endFrame();
}

void MainUiRenderer::drawStatusBar(uint32_t nowMs)
{
    char buffer[32];
    m_display.fillRect(0, 0, Layout::kScreenW, 26, Color::kHeader);
    m_display.setTextColor(Color::kText, Color::kHeader);

    const bool blinkOn = (nowMs / 500U) % 2U == 0U;

    if (m_model.screen == UiScreen::Main && m_model.activePage == 0U) {
        m_display.setTextSize(1);
        std::time_t nowTime = std::time(nullptr);
        std::tm* tmv = std::localtime(&nowTime);
        if (tmv && tmv->tm_year >= 120) {
            std::snprintf(buffer, sizeof(buffer), blinkOn ? "%04d-%02d-%02d %02d:%02d"
                                                          : "%04d-%02d-%02d %02d %02d",
                          tmv->tm_year + 1900, tmv->tm_mon + 1, tmv->tm_mday,
                          tmv->tm_hour, tmv->tm_min);
        } else {
            uint32_t totalSec = m_model.uptimeMs / 1000U;
            uint32_t hr = (totalSec / 3600U) % 24U;
            uint32_t mn = (totalSec / 60U) % 60U;
            std::snprintf(buffer, sizeof(buffer), blinkOn ? "00-00 %02u:%02u"
                                                          : "00-00 %02u %02u", hr, mn);
        }
        m_display.drawText(8, 5, buffer);

        const bool isFault = m_model.tempSensorFault || m_model.humiSensorFault;
        const bool isAlarm = (m_model.uptimeMs > 3000U) && (m_model.tempAlarm || m_model.humiAlarm);
        const bool isWarning = m_model.tempSensorWarning || m_model.humiSensorWarning;
        if ((isFault || isAlarm || isWarning) && blinkOn) {
            const char* txt = isFault ? "Sensor fault" : (isAlarm ? "Alarm" : "Warning");
            uint32_t color = (isFault || isAlarm) ? Color::kDanger : Color::kWarn;
            m_display.setTextColor(color, Color::kHeader);
            m_display.drawText(160, 7, txt);
        }
    } else {
        m_display.setTextSize(1);
        m_display.drawText(10, 6, screenTitle(m_model));
        formatDate(buffer, sizeof(buffer));
        m_display.setTextColor(Color::kTextDim, Color::kHeader);
        m_display.drawText(236, 6, buffer);
    }

    m_display.setTextSize(1);
    m_display.drawLine(0, 25, Layout::kScreenW, 25, Color::kDivider);
}

void MainUiRenderer::renderHeader(uint32_t) {}

void MainUiRenderer::renderFooter(uint32_t nowMs)
{
    m_display.drawLine(0, 215, Layout::kScreenW, 215, Color::kDivider);
    m_display.fillRect(0, 216, Layout::kScreenW, 24, Color::kFooter);

    if (m_model.screen == UiScreen::Main) {
        drawStatusIcons(nowMs);
        const char* info = m_model.sessionActive ? "Active" : "Idle";
        m_display.setTextSize(1);
        m_display.setTextColor(m_model.sessionActive ? Color::kOnIcon : Color::kTextDim,
                               Color::kFooter);
        m_display.drawText(260, 223, info);
    } else {
        const char* hint = "Turn: move  Click: select  Hold: back";
        if (m_model.screen == UiScreen::Manual) hint = "Turn: select/Hz  Click: edit/toggle  Hold: exit";
        if (m_model.screen == UiScreen::Rs485Test) hint = "Click: run  Hold: manual";
        if (m_model.screen == UiScreen::FactoryReset) hint = "Hold 10 sec: run  Hold: back";
        m_display.setTextSize(1);
        m_display.setTextColor(Color::kTextDim, Color::kFooter);
        m_display.drawText(12, 223, hint);
    }
}

void MainUiRenderer::renderPage1()
{
    char buffer[48];
    const int startY = Layout::kBodyY + 12;
    const int lineGap = 34;

    m_display.setTextSize(1);
    m_display.setTextColor(Color::kText, Color::kBg);
    m_display.drawText(20, startY, "Target Temp");
    std::snprintf(buffer, sizeof(buffer), "%.1f C", m_model.targetTempC);
    m_display.setTextColor(Color::kAccentTemp, Color::kBg);
    m_display.drawText(100, startY, buffer);

    m_display.setTextColor(Color::kText, Color::kBg);
    m_display.drawText(20, startY + lineGap, "Target Humi");
    std::snprintf(buffer, sizeof(buffer), "%.0f %%", m_model.targetHumidPct);
    m_display.setTextColor(Color::kAccentHumi, Color::kBg);
    m_display.drawText(100, startY + lineGap, buffer);

    m_display.setTextColor(Color::kText, Color::kBg);
    m_display.drawText(20, startY + lineGap * 2, "LED");
    m_display.setTextColor(m_model.ledOn ? Color::kOnIcon : Color::kOffIcon, Color::kBg);
    std::snprintf(buffer, sizeof(buffer), m_model.ledOn ? "%u%%" : "OFF", m_model.ledBrightnessPct);
    m_display.drawText(100, startY + lineGap * 2, buffer);

    m_display.setTextColor(Color::kText, Color::kBg);
    m_display.drawText(20, startY + lineGap * 3, "Ambient Lux");
    std::snprintf(buffer, sizeof(buffer), "%u lux", m_model.ambientLux);
    m_display.setTextColor(Color::kTextDim, Color::kBg);
    m_display.drawText(100, startY + lineGap * 3, buffer);
}

void MainUiRenderer::renderConfirm(const char* title, const char* line1, const char* line2)
{
    m_display.setTextSize(1);
    m_display.setTextColor(Color::kAccentTemp, Color::kBg);
    m_display.drawText(34, 60, title);
    m_display.setTextColor(Color::kText, Color::kBg);
    m_display.drawText(34, 82, line1);
    m_display.setTextColor(Color::kTextDim, Color::kBg);
    m_display.drawText(34, 104, line2);
    drawRow(148, "No", "Cancel", m_model.confirmCursor == 0, Color::kOffIcon);
    drawRow(180, "Yes", "Run", m_model.confirmCursor == 1, Color::kDanger);
}

void MainUiRenderer::drawSignalBars(int x, int y, bool connected)
{
    const uint32_t offColor = 0x5AEBU;
    const uint32_t onColor = 0xBDD7U;
    for (uint8_t i = 0; i < 4; ++i) {
        const int h = 4 + static_cast<int>(i) * 3;
        m_display.fillRect(x + static_cast<int>(i) * 4, y + 14 - h, 2, h,
                           connected ? onColor : offColor);
    }
}

void MainUiRenderer::drawCloudIcon(int x, int y, bool connected)
{
    const uint32_t color = connected ? 0xBDD7U : 0x5AEBU;
    m_display.fillRect(x + 4, y + 6, 12, 6, color);
    m_display.fillRect(x + 1, y + 8, 16, 4, color);
    m_display.fillRect(x + 7, y + 3, 6, 4, color);
}

void MainUiRenderer::drawProgressBar(int x, int y, int w, int h, uint8_t pct, uint32_t color)
{
    if (pct > 100U) pct = 100U;
    int fillW = (w - 4) * static_cast<int>(pct) / 100;
    m_display.drawRect(x, y, w, h, Color::kPanelSoft);
    m_display.fillRect(x + 2, y + 2, w - 4, h - 4, 0x0000U);
    if (fillW > 0) m_display.fillRect(x + 2, y + 2, fillW, h - 4, color);
}

void MainUiRenderer::drawPill(int x, int y, int w, const char* label, uint32_t color)
{
    m_display.drawRect(x, y, w, 20, color);
    m_display.setTextSize(1);
    m_display.setTextColor(Color::kText, Color::kBg);
    m_display.drawText(x + (hasUtf8Text(label) ? 8 : 6), y + 4, label);
}

void MainUiRenderer::drawButton(int x, int y, int w, int h, const char* label, uint32_t bgColor)
{
    m_display.fillRect(x, y, w, h, bgColor);
    m_display.drawRect(x, y, w, h, Color::kPanelSoft);
    m_display.setTextSize(1);
    m_display.drawText(x + (w / 2) - (std::strlen(label) * 3), y + (h / 2) - 4, label);
}

void MainUiRenderer::drawRow(int y, const char* label, const char* value,
                             bool selected, uint32_t valColor, int valSize)
{
    uint32_t bg = selected ? Color::kText : Color::kPanel;
    uint32_t border = selected ? Color::kText : Color::kPanelSoft;
    uint32_t textFg = selected ? Color::kBg : Color::kText;

    m_display.fillRect(12, y, 296, 27, bg);
    m_display.drawRect(12, y, 296, 27, border);
    m_display.setTextSize(1);
    m_display.setTextColor(textFg, bg);
    m_display.drawText(20, y + 7, label);

    m_display.setTextColor(selected ? Color::kBg : valColor, bg);
    m_display.setTextSize(valSize);
    if (valSize == 2) {
        m_display.drawText(270 - (std::strlen(value) * 12), y + 6, value);
    } else {
        m_display.drawText(270 - (std::strlen(value) * 6), y + 7, value);
    }
}

} // namespace growbed::ui
