#include "ui/pages/PageGrowTrayTest.h"

#include <cstdio>

namespace growbed::ui::pages
{
    void PageGrowTrayTest::render(uint32_t)
    {
        char stepText[20];
        std::snprintf(stepText, sizeof(stepText),
                      m_model.editMode ? "%lu Hz *" : "%lu Hz",
                      static_cast<unsigned long>(m_model.trayStepHz));

        drawRow(54, "On/Off",
                m_model.trayOn ? "RUN" : "STOP",
                m_model.manualCursor == 0,
                (m_model.trayOn && m_model.manualCursor == 0) ? Color::kAccentHumi : Color::kDanger, 1);

        drawRow(88, "Step Hz",
                stepText,
                m_model.manualCursor == 1,
                m_model.editMode ? Color::kAccentTemp : Color::kTextDim, 1);

        drawRow(122, "Left Touch",
                m_model.trayLeftTouchTest ? "Left Touch" : "",
                m_model.manualCursor == 2,
                m_model.trayLeftTouchTest ? Color::kOnIcon : Color::kTextDim, 1);

        drawRow(156, "Right Touch",
                m_model.trayRightTouchTest ? "Right Touch" : "",
                m_model.manualCursor == 3,
                m_model.trayRightTouchTest ? Color::kOnIcon : Color::kTextDim, 1);
    }
}
