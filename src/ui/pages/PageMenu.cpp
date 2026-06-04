#include "ui/pages/PageMenu.h"

namespace growbed::ui::pages
{
    void PageMenu::render(uint32_t)
    {
        constexpr uint8_t kMaxVisible = 5;
        uint8_t topIndex = 0;

        if (m_model.menuCursor >= kMaxVisible) {
            topIndex = m_model.menuCursor - kMaxVisible + 1;
        }

        for (uint8_t v = 0; v < kMaxVisible; ++v) {
            const uint8_t i = topIndex + v;
            if (i >= kMainMenuCount) break;

            const int y = 54 + static_cast<int>(v) * 31;
            const bool selected = (m_model.menuCursor == i);

            const uint32_t bg = selected ? Color::kText : Color::kPanel;
            const uint32_t border = selected ? Color::kText : Color::kPanelSoft;
            const uint32_t textFg = selected ? Color::kBg : Color::kText;
            const uint32_t descFg = selected ? Color::kBg : Color::kTextDim;

            m_display.fillRect(12, y, 296, 27, bg);
            m_display.drawRect(12, y, 296, 27, border);

            m_display.setTextSize(1);
            m_display.setTextColor(textFg, bg);
            m_display.drawText(24, y + 7, kMainMenuItems[i].name);

            m_display.setTextColor(descFg, bg);
            m_display.drawText(198, y + 7, kMainMenuItems[i].desc);
        }
    }
}
