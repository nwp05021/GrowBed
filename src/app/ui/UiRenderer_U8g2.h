#pragma once
#include <Arduino.h>
#include <U8g2lib.h>
#include "../controllers/MotionController.h"
#include "UiIcons.h"

enum class UiScreen : uint8_t {
    Main = 0,
    MenuRoot,
    MenuMotion,
    MenuParams,
    MenuDiag,
    MenuSystem,
    EditValue,
    Engineering
};

struct UiViewModel {
    bool envValid = false;
    float tempC = 0;
    float humPct = 0;

    MotionStatus st;
    MotionConfig cfg;

    UiScreen screen = UiScreen::Main;
    uint8_t cursor = 0;
    uint8_t page = 0;
    bool invert = false;
    bool blink = false;

    uint32_t uptimeMs = 0;

    const char* editLabel = nullptr;
    int32_t editValue = 0;
    int32_t editMin = 0;
    int32_t editMax = 0;
    const char* editUnit = nullptr;

    // fault
    bool isFault = false;
    uint8_t faultCode = 0;
    uint8_t retryCount = 0;
    const char* faultTitle = nullptr;
    const char* faultDetail = nullptr;

    // diagnostic
    uint32_t faultTotal = 0;
    uint32_t lastFaultUptimeMs = 0;
    uint8_t  recoverAttempts = 0;
    uint8_t  lastFaultCode = 0;
    bool     permanentFault = false;

    uint32_t resetCount = 0;
};

class UiRenderer_U8g2 {
public:
    void begin() {
        u8g2.begin();
        u8g2.clearDisplay();   // 🔥 이거 중요
        u8g2.setContrast(200);

        u8g2.enableUTF8Print();
        u8g2.setFontMode(1);
    }

    void draw(const UiViewModel& vm) {
        if (vm.isFault) {
            drawFault(vm);
            return;
        }

        u8g2.clearBuffer();
        u8g2.setDrawColor(1); // 🔥 항상 1로 시작 (우측 바/잔상 방지)

        switch (vm.screen) {
            case UiScreen::Main: drawMain(vm); break;
            case UiScreen::MenuRoot:  drawMenuRoot(vm); break;
            case UiScreen::MenuMotion:drawMenuMotion(vm); break;
            case UiScreen::MenuParams:drawMenuParams(vm); break;
            case UiScreen::MenuDiag:  drawMenuDiag(vm); break;
            case UiScreen::MenuSystem:drawMenuSystem(vm); break;
            case UiScreen::EditValue: drawEdit(vm); break;
            case UiScreen::Engineering: drawEngineering(vm); break;
            default: drawMain(vm); break;
        }

        u8g2.setDrawColor(1); // 🔥 항상 복구
        u8g2.sendBuffer();
    }

private:
    U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2 =
        U8G2_SH1106_128X64_NONAME_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE);

    /* ---------------- Header ---------------- */

    void drawHeader(const UiViewModel& vm) {
        u8g2.setFont(u8g2_font_6x10_tf);

        uint32_t s = vm.uptimeMs / 1000;
        uint32_t m = s / 60;
        uint32_t h = m / 60;

        char buf[24];
        snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu",
                 h % 24, m % 60, s % 60);

        u8g2.drawStr(2, 10, buf);

        if (vm.st.state == MotionState::Fault) {
            u8g2.drawStr(120, 10, "!");
        } else {
            char pbuf[8];
            snprintf(pbuf, sizeof(pbuf), "P%u", vm.page + 1);
            u8g2.drawStr(112, 10, pbuf);        
        }

        u8g2.drawHLine(0, 12, 128);
    }

    /* ---------------- Body ---------------- */
    void drawMainPages(const UiViewModel& vm) {
        char line1[32];
        char line2[32];

        switch (vm.page) {
            case 0: {
                if (vm.envValid) {
                    snprintf(line1, sizeof(line1), "온도:%5.1f C", vm.tempC);
                    snprintf(line2, sizeof(line2), "습도:%5.1f %%", vm.humPct);
                } else {
                    snprintf(line1, sizeof(line1), "온도: --.- C");
                    snprintf(line2, sizeof(line2), "습도: --.- %%");
                }
                break;
            }
            case 1: { // Motion detail
                snprintf(line1, sizeof(line1), "SPS %d", (int)vm.st.currentSps);
                snprintf(line2, sizeof(line2), "POS %ld", (long)vm.st.pos);
                break;
            }
            case 2: { // Hall detail
                snprintf(line1, sizeof(line1), "L %u/%u",
                        (unsigned)vm.st.hallRawL, (unsigned)(vm.st.hallL ? 1 : 0));
                snprintf(line2, sizeof(line2), "R %u/%u",
                        (unsigned)vm.st.hallRawR, (unsigned)(vm.st.hallR ? 1 : 0));
                break;
            }
            default:
                snprintf(line1, sizeof(line1), "-");
                snprintf(line2, sizeof(line2), "-");
                break;
        }

        // 줄 간격 약간 넓힘
        u8g2.drawUTF8(2, 28, line1);
        u8g2.drawUTF8(2, 48, line2);
    }

    void drawBody(const UiViewModel& vm) {
        const bool isFault = (vm.st.state == MotionState::Fault);

        // Body 구분선(하단)은 항상 그릴 거라, Body 영역은 y=13..51 (39px)
        if (isFault) {
            // Body만 반전
            u8g2.drawBox(0, 13, 130, 39);
            u8g2.setDrawColor(0);
        } else {
            u8g2.setDrawColor(1);
        }

        // Body 폰트(한글)
        u8g2.setFont(u8g2_font_unifont_t_korean2);

        if (isFault) {
            drawFault(vm);
        } else {
            // ✅ Main에서만 page를 활용 (다른 screen은 기존대로)
            if (vm.screen == UiScreen::Main) {
                drawMainPages(vm);
            } else {
                // fallback: 기존 메인표시와 동일
                drawMainPages(vm);
            }
        }

        // Footer 구분선
        u8g2.setDrawColor(1);
        u8g2.drawHLine(0, 52, 128);
    }

    void drawFault(const UiViewModel& vm) {
        u8g2.clearBuffer();

        u8g2.setDrawColor(1);
        u8g2.drawBox(0, 0, 128, 64);      // full invert
        u8g2.setDrawColor(0);

        u8g2.setFont(u8g2_font_6x12_tr);

        char buf[20];
        snprintf(buf, sizeof(buf), "FAULT %02d", vm.faultCode);
        u8g2.drawStr(10, 16, buf);

        u8g2.drawStr(10, 30, vm.faultTitle);

        u8g2.drawHLine(0, 36, 128);

        char retry[20];
        snprintf(retry, sizeof(retry), "Retry %d/3", vm.retryCount);
        u8g2.drawStr(10, 50, retry);

        if (vm.blink)
            u8g2.drawStr(70, 50, "Hold to reset");

        u8g2.sendBuffer();
    }

    /* ---------------- Footer ---------------- */

    void drawFooter(const UiViewModel& vm) {
        int y = 54;   // Footer 영역
        int x = 8;

        // Motor
        if (vm.st.state == MotionState::MoveLeft ||
            vm.st.state == MotionState::MoveRight)
            u8g2.drawXBM(x, y, 12, 12, ICON_MOTOR);
        x += 28;

        // Hall
        if (vm.st.hallL || vm.st.hallR)
            u8g2.drawXBM(x, y, 12, 12, ICON_HALL);
        x += 28;

        // WiFi (임시 항상 표시)
        u8g2.drawXBM(x, y, 12, 12, ICON_WIFI);
        x += 28;

        // Error
        if (vm.st.err != MotionError::None)
            u8g2.drawXBM(x, y, 12, 12, ICON_ERROR);
    }

    /* ---------------- Main ---------------- */

    void drawMain(const UiViewModel& vm) {
        drawHeader(vm);
        drawBody(vm);
        drawFooter(vm);
    }

    /* ---------------- Menu Common ---------------- */

    void drawMenuHeader(const UiViewModel& vm, const char* title) {
        // Menu header uses the SAME top bar (no "double header").
        // This also moves the menu content up (user request: ~14px).
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawUTF8(2, 10, title);

        if (vm.st.state == MotionState::Fault) {
            u8g2.drawStr(120, 10, "!");
        }
        u8g2.drawHLine(0, 12, 128);
    }

    void drawMenuList(const char* const* items, uint8_t count, uint8_t cursor) {
        // Use small font to fit 4 menu rows cleanly.
        u8g2.setFont(u8g2_font_6x10_tf);

        // List area starts right under header line (y=12)
        const int16_t y0 = 26;
        const int16_t lineH = 12;
        const uint8_t visible = 4;

        int start = 0;
        if (count > visible) {
            // center cursor in view when possible
            if (cursor == 0) start = 0;
            else if (cursor >= count - 1) start = (int)count - (int)visible;
            else start = (int)cursor - 1;
        }

        for (uint8_t i = 0; i < visible; i++) {
            uint8_t idx = (uint8_t)(start + i);
            if (idx >= count) break;
            int16_t y = y0 + (int16_t)i * lineH;

            // highlight
            if (idx == cursor) {
                u8g2.setDrawColor(1);
                u8g2.drawBox(0, y - 10, 128, 12);
                u8g2.setDrawColor(0);
            } else {
                u8g2.setDrawColor(1);
            }

            u8g2.drawUTF8(4, y, items[idx]);
        }

        // restore
        u8g2.setDrawColor(1);
        u8g2.drawHLine(0, 52, 128);
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(2, 63, "Click:Select  Long:Back");
    }

    /* ---------------- Menu: Root ---------------- */

    void drawMenuRoot(const UiViewModel& vm) {
        drawMenuHeader(vm, "MENU");

        static const char* const items[] = {
            "Motion",
            "Parameters",
            "Diagnostics",
            "System"
        };
        drawMenuList(items, 4, vm.cursor);
    }

    /* ---------------- Menu: Motion ---------------- */

    void drawMenuMotion(const UiViewModel& vm) {
        drawMenuHeader(vm, "Motion");

        static const char* const items[] = {
            "Start",
            "Stop",
            "Home",
            "Recalibrate"
        };
        drawMenuList(items, 4, vm.cursor);
    }

    /* ---------------- Menu: Params ---------------- */

    void drawMenuParams(const UiViewModel& vm) {
        drawMenuHeader(vm, "Parameters");

        // show live values in-line (short)
        char b0[24], b1[24], b2[24], b3[24];
        snprintf(b0, sizeof(b0), "MaxSps: %d", (int)vm.cfg.maxSps);
        snprintf(b1, sizeof(b1), "Accel : %d", (int)vm.cfg.accel);
        snprintf(b2, sizeof(b2), "Dwell : %dms", (int)vm.cfg.dwellMs);
        snprintf(b3, sizeof(b3), "Rehome: %d", (int)vm.cfg.rehomeEveryCycles);

        const char* items[] = { b0, b1, b2, b3 };
        drawMenuList(items, 4, vm.cursor);
    }

    /* ---------------- Menu: Diagnostics ---------------- */

    void drawMenuDiag(const UiViewModel& vm) {
        drawMenuHeader(vm, "Diagnostics");
        u8g2.setFont(u8g2_font_6x10_tf);

        char l1[32] = {0}, l2[32] = {0}, l3[32] = {0}, l4[32] = {0};

        switch (vm.page) {
            case 0: {
                snprintf(l1, sizeof(l1), "FaultTotal : %lu", (unsigned long)vm.faultTotal);
                snprintf(l2, sizeof(l2), "LastCode   : %u",  (unsigned)vm.lastFaultCode);
                snprintf(l3, sizeof(l3), "LastMs     : %lu", (unsigned long)vm.lastFaultUptimeMs);
                snprintf(l4, sizeof(l4), "Permanent  : %u",  (unsigned)vm.permanentFault);
                break;
            }
            case 1: {
                snprintf(l1, sizeof(l1), "Hall L:%u/%u R:%u/%u",
                    (unsigned)vm.st.hallRawL, (unsigned)(vm.st.hallL ? 1 : 0),
                    (unsigned)vm.st.hallRawR, (unsigned)(vm.st.hallR ? 1 : 0));
                snprintf(l2, sizeof(l2), "POS:%ld SPS:%d", (long)vm.st.pos, (int)vm.st.currentSps);
                snprintf(l3, sizeof(l3), "Cycles     : %lu", (unsigned long)vm.st.cycles);
                snprintf(l4, sizeof(l4), "RecoverTry : %u",  (unsigned)vm.st.recoverAttempts);
                break;
            }
            case 2: {
                snprintf(l1, sizeof(l1), "State:%u Err:%u", (unsigned)vm.st.state, (unsigned)vm.st.err);
                snprintf(l2, sizeof(l2), "TravelSteps: %lu", (unsigned long)vm.st.travelSteps);
                snprintf(l3, sizeof(l3), "Cfg MaxSps : %d", (int)vm.cfg.maxSps);
                snprintf(l4, sizeof(l4), "Cfg Accel  : %d", (int)vm.cfg.accel);
                break;
            }
            default: {
                snprintf(l1, sizeof(l1), "-");
                snprintf(l2, sizeof(l2), "-");
                snprintf(l3, sizeof(l3), "-");
                snprintf(l4, sizeof(l4), "-");
                break;
            }
        }

        u8g2.drawUTF8(2, 25, l1);
        u8g2.drawUTF8(2, 37, l2);
        u8g2.drawUTF8(2, 49, l3);
        u8g2.drawUTF8(2, 61, l4);

        // Page indicator (right-bottom)
        char pbuf[12];
        snprintf(pbuf, sizeof(pbuf), "%u/3", (unsigned)(vm.page + 1));
        u8g2.drawStr(106, 63, pbuf);
    }

    /* ---------------- Menu: System ---------------- */

    void drawMenuSystem(const UiViewModel& vm) {
        drawMenuHeader(vm, "System");

        static const char* const items[] = {
            "About",
            "Back"
        };
        drawMenuList(items, 2, vm.cursor);
    }

    /* ---------------- Engineering ---------------- */

    void drawEngineering(const UiViewModel& vm) {
        drawMenuHeader(vm, "Engineering");

        static const char* const items[] = {
            "Force Home",
            "Move Left",
            "Move Right",
            "Disable Motor"
        };

        drawMenuList(items, 4, vm.cursor);

        // override footer hint
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(2, 63, "Click:Run  Long:Back");
    }

    /* ---------------- Edit ---------------- */

    void drawEdit(const UiViewModel& vm) {
        drawHeader(vm);

        u8g2.setFont(u8g2_font_unifont_t_korean2);
        if (vm.editLabel)
            u8g2.drawUTF8(4, 30, vm.editLabel);

        char buf[24];
        snprintf(buf, sizeof(buf), "%ld%s",
                 (long)vm.editValue,
                 vm.editUnit ? vm.editUnit : "");

        u8g2.setFont(u8g2_font_logisoso20_tf);
        int16_t w = u8g2.getStrWidth(buf);
        u8g2.drawStr((128 - w) / 2, 48, buf);

        u8g2.drawHLine(0, 52, 128);

        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(2, 63, "Click:Save  LongClick:Cancel");
    }

    /* ---------------- Text Helpers ---------------- */

    const char* stateText(MotionState s) {
        switch (s) {
            case MotionState::MoveRight: return "▶ 우측 이동";
            case MotionState::MoveLeft:  return "◀ 좌측 이동";
            case MotionState::Dwell:     return "■ 대기";
            case MotionState::HomingLeft:return "H 초기화";
            case MotionState::RecoverWait:return "복구 대기";
            case MotionState::Stopped:   return "□ 정지";
            case MotionState::Fault:     return "! 오류";
            default: return "";
        }
    }

    const char* errorText(MotionError e) {
        switch (e) {
            case MotionError::HomingTimeout:    return "홈 위치 실패";
            case MotionError::TravelTimeout:    return "이동 시간 초과";
            case MotionError::CalibFailed:      return "보정 실패";
            case MotionError::BothLimitsActive: return "양쪽 센서 충돌";
            case MotionError::None:
            default: return "";
        }
    }
};
