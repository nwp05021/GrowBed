#pragma once
#include "ui/UiModel.h"
#include "devices/St7789Display.h"
#include <cstdint>

// RP2040 ?¬íŠ¸: ProvisioningRenderer ?¤í…
// ProvisioningManagerê°€ ??ƒ ë¹„í™œ?±ì´ë¯€ë¡?render()???¸ì¶œ?˜ì? ?ŠìŠµ?ˆë‹¤.
namespace growbed::ui
{
    class ProvisioningRenderer
    {
    public:
        ProvisioningRenderer(UiModel& model, growbed::devices::St7789Display& display)
            : m_model(model), m_display(display) {}

        void reset()             {}
        void render(uint32_t)    {}

    private:
        UiModel&                           m_model;
        growbed::devices::St7789Display& m_display;
    };
}
