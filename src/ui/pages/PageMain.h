#pragma once
#include "ui/pages/BasePage.h"

namespace growbed::ui::pages
{
    class PageMain : public BasePage
    {
    public:
        using BasePage::BasePage; // ë¶€ëª??ì„±???ì†
        void render(uint32_t nowMs) override;
    };
}