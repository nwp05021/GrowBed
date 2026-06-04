#include "ui/pages/PageStartDate.h"
#include "ui/UiColors.h"
#include <cstdio>

namespace growbed::ui::pages
{
    void PageStartDate::render(uint32_t)
    {
        char dateStr[8];
        static constexpr const char* labels[] = { "Year", "Month", "Day" };
        const int values[] = {
            static_cast<int>(m_model.editBatchYear),
            static_cast<int>(m_model.editBatchMonth),
            static_cast<int>(m_model.editBatchDay),
        };

        for (int i = 0; i < 3; ++i) {
            const int x = 16 + i * 98;
            const int y = 60;
            const bool fieldSel = (m_model.fieldCursor == static_cast<uint8_t>(i));

            const uint32_t valBg = fieldSel ? Color::kText : Color::kPanel;
            const uint32_t valFg = fieldSel ? Color::kBg : Color::kText;
            const uint32_t boxBorder = fieldSel ? Color::kText : Color::kPanelSoft;

            m_display.setTextSize(1);
            m_display.setTextColor(Color::kTextDim, Color::kBg);
            m_display.drawText(x + 30, y + 6, labels[i]);

            m_display.fillRect(x, y + 20, 84, 56, valBg);
            m_display.drawRect(x, y + 20, 84, 56, boxBorder);

            if (i == 0) std::snprintf(dateStr, sizeof(dateStr), "%04d", values[i]);
            else        std::snprintf(dateStr, sizeof(dateStr), "%02d", values[i]);

            m_display.setTextSize(2);
            m_display.setTextColor(valFg, valBg);
            m_display.drawText((i == 0) ? x + 18 : x + 30, y + 40, dateStr);
        }

        const uint32_t saveBg = (m_model.fieldCursor == 3U) ? Color::kText : Color::kPanel;
        const uint32_t saveFg = (m_model.fieldCursor == 3U) ? Color::kBg : Color::kText;
        m_display.setTextColor(saveFg, saveBg);
        drawButton(20, 164, 130, 32, "Save", saveBg);

        const uint32_t cancelBg = (m_model.fieldCursor == 4U) ? Color::kText : Color::kPanel;
        const uint32_t cancelFg = (m_model.fieldCursor == 4U) ? Color::kBg : Color::kText;
        m_display.setTextColor(cancelFg, cancelBg);
        drawButton(170, 164, 130, 32, "Cancel", cancelBg);

        if (m_model.editMode) {
            m_display.setTextSize(1);
            m_display.setTextColor(Color::kWarn, Color::kBg);
            m_display.drawText(92, 204, "Editing");
        }
    }
}
