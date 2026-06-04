#pragma once

#include "ui/pages/BasePage.h"

namespace growbed::ui::pages
{
    class PageGrowTrayTest : public BasePage
    {
    public:
        PageGrowTrayTest(UiModel& model, growbed::devices::St7789Display& display)
            : BasePage(model, display) {}

        void render(uint32_t nowMs) override;
    };
}
