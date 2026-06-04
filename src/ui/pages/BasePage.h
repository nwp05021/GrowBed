#pragma once
#include "ui/UiModel.h"
#include "devices/St7789Display.h"
#include "ui/UiLayout.h"
#include "ui/UiColors.h"

namespace growbed::ui::pages
{
    class BasePage
    {
    protected:
        UiModel& m_model;
        growbed::devices::St7789Display& m_display;

    public:
        BasePage(UiModel& model, growbed::devices::St7789Display& display)
            : m_model(model), m_display(display) {}
        
        virtual ~BasePage() = default;

        // ê°??˜ì´ì§€ê°€ ë°˜ë“œ??êµ¬í˜„?´ì•¼ ?˜ëŠ” ?Œë”ë§??¨ìˆ˜
        virtual void render(uint32_t nowMs) = 0;

    protected:
        // ê³µí†µ UI ? í‹¸ë¦¬í‹° (MainUiRenderer?ì„œ ?´ë™)
        bool hasUtf8(const char* text);
        void drawProgressBar(int x, int y, int w, int h, uint8_t pct, uint32_t color);
        void drawPill(int x, int y, int w, const char* label, uint32_t color);
        void drawButton(int x, int y, int w, int h, const char* label, uint32_t bgColor);
        void drawRow(int y, const char* label, const char* value, bool selected = false, uint32_t valColor = 0, int valSize = 1);

        void renderConfirm(const char*, const char* line1, const char* line2);
        void drawStatusIcons(uint32_t nowMs);
        const char* presetName(uint8_t i);
    };
}