#include "ui/pages/PageHelp.h"
#include "ui/UiColors.h"
#include <cstdio>

namespace growbed::ui::pages
{
    void PageHelp::render(uint32_t nowMs)
    {
        char buf[48];
        const int startY  = 44;
        const int lineGap = 26;

        m_display.setTextSize(1);

        m_display.setTextColor(Color::kText, Color::kBg);
        m_display.drawText(16, startY, "?úÏä§???ïÎ≥¥");

        m_display.drawLine(16, startY + lineGap - 4, 304, startY + lineGap - 4, Color::kDivider);

        // Í∞Ä???úÍ∞Ñ
        uint32_t totalSec = m_model.uptimeMs / 1000U;
        uint32_t hr  = (totalSec / 3600U);
        uint32_t min = (totalSec % 3600U) / 60U;
        uint32_t sec = totalSec % 60U;
        std::snprintf(buf, sizeof(buf), "%02u:%02u:%02u", hr, min, sec);
        m_display.setTextColor(Color::kTextDim, Color::kBg);
        m_display.drawText(16, startY + lineGap,       "Í∞Ä???úÍ∞Ñ");
        m_display.setTextColor(Color::kText, Color::kBg);
        m_display.drawText(160, startY + lineGap, buf);

        // Î∂Ä???üÏàò
        std::snprintf(buf, sizeof(buf), "#%u", static_cast<unsigned>(m_model.bootCount));
        m_display.setTextColor(Color::kTextDim, Color::kBg);
        m_display.drawText(16, startY + lineGap * 2,  "Î∂Ä???üÏàò");
        m_display.setTextColor(Color::kText, Color::kBg);
        m_display.drawText(160, startY + lineGap * 2, buf);

        // ?ºÏÑú ?ÅÌÉú
        m_display.setTextColor(Color::kTextDim, Color::kBg);
        m_display.drawText(16, startY + lineGap * 3,  "?ºÏÑú ?ÅÌÉú");
        bool sensorOk = !m_model.tempSensorFault && !m_model.humiSensorFault;
        m_display.setTextColor(sensorOk ? Color::kOnIcon : Color::kDanger, Color::kBg);
        m_display.drawText(160, startY + lineGap * 3, sensorOk ? "?ïÏÉÅ" : "?§Î•ò");

        // FW Î≤ÑÏ†Ñ
        m_display.setTextColor(Color::kTextDim, Color::kBg);
        m_display.drawText(16, startY + lineGap * 4,  "FW Î≤ÑÏ†Ñ");
        m_display.setTextColor(Color::kText, Color::kBg);
        m_display.drawText(160, startY + lineGap * 4, m_model.fwVersion);
    }
}
