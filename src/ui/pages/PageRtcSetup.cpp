#include "ui/pages/PageRtcSetup.h"
#include "ui/UiColors.h"
#include <cstdio>

namespace growbed::ui::pages
{
    void PageRtcSetup::render(uint32_t)
    {
        static constexpr const char* labels[] = {
            "Year", "Month", "Day", "Hour", "Minute", "Second"
        };
        const int values[] = {
            static_cast<int>(m_model.editRtcYear),
            static_cast<int>(m_model.editRtcMonth),
            static_cast<int>(m_model.editRtcDay),
            static_cast<int>(m_model.editRtcHour),
            static_cast<int>(m_model.editRtcMinute),
            static_cast<int>(m_model.editRtcSecond),
        };

        char value[8];
        for (int i = 0; i < 6; ++i) {
            const int column = i % 3;
            const int row = i / 3;
            const int x = 16 + column * 98;
            const int y = 34 + row * 62;
            const bool selected = m_model.fieldCursor == static_cast<uint8_t>(i);
            const uint32_t bg = selected ? Color::kText : Color::kPanel;
            const uint32_t fg = selected ? Color::kBg : Color::kText;
            const uint32_t border = selected ? Color::kText : Color::kPanelSoft;

            m_display.setTextSize(1);
            m_display.setTextColor(Color::kTextDim, Color::kBg);
            m_display.drawText(x + 24, y, labels[i]);

            m_display.fillRect(x, y + 15, 84, 38, bg);
            m_display.drawRect(x, y + 15, 84, 38, border);
            if (i == 0) std::snprintf(value, sizeof(value), "%04d", values[i]);
            else        std::snprintf(value, sizeof(value), "%02d", values[i]);

            m_display.setTextSize(2);
            m_display.setTextColor(fg, bg);
            m_display.drawText(i == 0 ? x + 18 : x + 30, y + 20, value);
        }

        const bool saveSelected = m_model.fieldCursor == 6U;
        const uint32_t saveBg = saveSelected ? Color::kText : Color::kPanel;
        m_display.setTextColor(saveSelected ? Color::kBg : Color::kText, saveBg);
        drawButton(20, 160, 130, 30, "Save", saveBg);

        const bool cancelSelected = m_model.fieldCursor == 7U;
        const uint32_t cancelBg = cancelSelected ? Color::kText : Color::kPanel;
        m_display.setTextColor(cancelSelected ? Color::kBg : Color::kText, cancelBg);
        drawButton(170, 160, 130, 30, "Cancel", cancelBg);

        m_display.setTextSize(1);
        if (m_model.actionMessage[0] != '\0') {
            const uint32_t messageBg =
                m_model.rtcSaveSucceeded ? Color::kOnIcon : Color::kDanger;
            m_display.fillRect(60, 194, 200, 18, messageBg);
            m_display.setTextColor(Color::kText, messageBg);
            m_display.drawText(106, 195, m_model.actionMessage);
        } else if (m_model.editMode) {
            m_display.setTextColor(Color::kWarn, Color::kBg);
            m_display.drawText(136, 196, "Editing");
        }
    }
}
