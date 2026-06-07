#include "ui/pages/PageHelp.h"
#include "ui/UiColors.h"
#include <cstdio>

namespace growbed::ui::pages
{
    void PageHelp::render(uint32_t)
    {
        char detail[48];
        int row = 0;

        auto drawAlarm = [&](const char* title, const char* value, uint32_t color) {
            constexpr int kBoxY = 30;
            constexpr int kBoxHeight = 38;
            constexpr int kRowPitch = 40;
            const int y = kBoxY + row * kRowPitch;

            m_display.fillRect(12, y, 296, kBoxHeight, Color::kPanel);
            m_display.drawRect(12, y, 296, kBoxHeight, color);
            m_display.setTextSize(1);
            m_display.setTextColor(color, Color::kPanel);
            m_display.drawText(22, y + 3, title);
            m_display.setTextColor(Color::kText, Color::kPanel);
            m_display.drawText(22, y + 20, value);
            ++row;
        };

        if (m_model.tempAlarm) {
            const char* direction =
                m_model.displayTempC >= m_model.targetTempC ? "HIGH" : "LOW";
            std::snprintf(detail, sizeof(detail), "%s  Now %.1f C / Target %.1f C",
                          direction, m_model.displayTempC, m_model.targetTempC);
            drawAlarm("Temperature alarm", detail, Color::kDanger);
        }

        if (m_model.humiAlarm) {
            const char* direction =
                m_model.displayHumidPct >= m_model.targetHumidPct ? "HIGH" : "LOW";
            std::snprintf(detail, sizeof(detail), "%s  Now %.0f %% / Target %.0f %%",
                          direction, m_model.displayHumidPct, m_model.targetHumidPct);
            drawAlarm("Humidity alarm", detail, Color::kDanger);
        }

        if (m_model.tempSensorFault) {
            drawAlarm("Temperature sensor", "FAULT: no valid measurement", Color::kDanger);
        } else if (m_model.tempSensorWarning) {
            drawAlarm("Temperature sensor", "WARNING: latest read failed", Color::kWarn);
        }

        if (m_model.humiSensorFault) {
            drawAlarm("Humidity sensor", "FAULT: no valid measurement", Color::kDanger);
        } else if (m_model.humiSensorWarning) {
            drawAlarm("Humidity sensor", "WARNING: latest read failed", Color::kWarn);
        }

        if (row == 0) {
            m_display.fillRect(24, 72, 272, 64, Color::kPanel);
            m_display.drawRect(24, 72, 272, 64, Color::kOnIcon);
            m_display.setTextSize(1);
            m_display.setTextColor(Color::kOnIcon, Color::kPanel);
            m_display.drawText(104, 88, "No active alarms");
            m_display.setTextColor(Color::kTextDim, Color::kPanel);
            m_display.drawText(90, 112, "All monitored values are normal");
        }

        std::snprintf(detail, sizeof(detail), "Boot #%u   FW %s",
                      static_cast<unsigned>(m_model.bootCount), m_model.fwVersion);
        m_display.setTextSize(1);
        m_display.setTextColor(Color::kTextDim, Color::kBg);
        m_display.drawText(16, 198, detail);
    }
}
