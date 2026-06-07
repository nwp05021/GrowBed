#pragma once
#include "ui/pages/BasePage.h"

namespace growbed::ui::pages
{
    class PageRtcSetup : public BasePage
    {
    public:
        using BasePage::BasePage;
        void render(uint32_t nowMs) override;
    };
}
