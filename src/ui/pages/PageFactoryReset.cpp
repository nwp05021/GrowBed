#include "ui/pages/PageFactoryReset.h"
#include "ui/UiColors.h"

namespace growbed::ui::pages
{
    void PageFactoryReset::render(uint32_t nowMs)
    {
        m_display.setTextSize(1);
        m_display.setTextColor(Color::kDanger, Color::kBg);
        m_display.drawText(24, 76, "Î™®Îì† ?§Ï†ï, WiFi, Î∂Ä??Í≥ÑÌöç????†ú?©Îãà??");
        m_display.setTextColor(Color::kTextDim, Color::kBg);
        m_display.drawText(24, 100, "?§Ìñâ?òÎ†§Î©?Î≤ÑÌäº??10Ï¥àÍ∞Ñ Í≥ÑÏÜç ?ÑÎ•¥?∏Ïöî.");
        drawProgressBar(32, 138, 256, 18, m_model.factoryProgressPct, Color::kDanger);
        char pct[16];
        std::snprintf(pct, sizeof(pct), "%u%%", m_model.factoryProgressPct);
        m_display.setTextColor(Color::kText, Color::kBg);
        m_display.drawText(144, 166, pct);
    }
}