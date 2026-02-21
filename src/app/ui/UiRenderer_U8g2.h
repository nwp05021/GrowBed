#pragma once

#include <Arduino.h>
#include <U8g2lib.h>

#include "UiModel.h"
#include "UiIcons.h"

class UiRenderer_U8g2 {
public:
    UiRenderer_U8g2();

    void begin();
    void draw(const UiViewModel& vm);

private:
    U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

    // Top-level pieces
    void drawMain(const UiViewModel& vm);
    void drawHeader(const UiViewModel& vm);
    void drawBody(const UiViewModel& vm);
    void drawFooter(const UiViewModel& vm);

    // Fault
    void drawFaultFullScreen(const UiViewModel& vm); // clear+invert+send
    void drawFaultOverlay(const UiViewModel& vm);    // draw into current buffer only

    // Main pages
    void drawMainPages(const UiViewModel& vm);

    // Screens
    void drawMenuRoot(const UiViewModel& vm);
    void drawMenuMotion(const UiViewModel& vm);
    void drawMenuParams(const UiViewModel& vm);
    void drawMenuDiag(const UiViewModel& vm);
    void drawMenuSystem(const UiViewModel& vm);
    void drawMenuLed(const UiViewModel& vm);
    void drawMenuTest(const UiViewModel& vm);
    void drawTestRunning(const UiViewModel& vm);
    void drawToast(const UiViewModel& vm);
    void drawAlertPopup(const UiViewModel& vm);
    void drawEngineering(const UiViewModel& vm);
    void drawEdit(const UiViewModel& vm);
    void drawTestResult(const UiViewModel& vm);

    // Menu common
    void drawMenuHeader(const UiViewModel& vm, const char* title);
    void drawMenuList(const char* const* items, uint8_t count, uint8_t cursor);

    // Helpers
    static void fmtHHMM(char* out, size_t outSz, uint16_t minutes);
    const char* stateText(MotionState s);
    const char* errorText(MotionError e);
};