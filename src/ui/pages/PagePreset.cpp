#include "ui/pages/PagePreset.h"
#include "domain/PlantSpecies.h"
#include "ui/UiColors.h"

namespace growbed::ui::pages
{
    void PagePreset::render(uint32_t)
    {
        if (m_model.presetConfirm) {
            renderConfirm("?ë¬¼ ?•ì±… ? íƒ",
                          "? íƒ???•ì±…???ìš©? ê¹Œ??",
                          "ëª©í‘œ ?¨ë„/?µë„/LED ê¸°ì???ë³€ê²½ë©?ˆë‹¤.");
            return;
        }

        for (uint8_t i = 0; i < domain::kPlantSpeciesCount; ++i) {
            char no[8];
            std::snprintf(no, sizeof(no), "%u", i + 1U);
            drawRow(74 + i * 42, no, presetName(i),
                    m_model.presetCursor == i, Color::kText, 1);
        }
    }
}
