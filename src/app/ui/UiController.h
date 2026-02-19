#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <cstring>

#include "../../config/PinMap.h"
#include "../../config/Defaults.h"
#include "../controllers/EncoderController.h"
#include "../controllers/MotionController.h"
#include "UiRenderer_U8g2.h"

// UiController owns UI state machine and converts encoder events into motion requests.
// It must NEVER directly mutate MotionStatus/state.

class UiController {
public:
    void begin(const UiConfig& cfg_, MotionController* motion_) {
        cfg = cfg_;
        motion = motion_;

        Wire.setSDA(PIN_I2C_SDA);
        Wire.setSCL(PIN_I2C_SCL);
        Wire.begin();

        renderer.begin();

        // AHT10/20
        envOk = aht.begin(&Wire);
        lastSensorMs = 0;
        lastDrawMs = 0;
        lastBlinkMs = 0;
        blink = false;
    }

    void handleEncoder(const EncoderEvents& e) {
        if (e.delta != 0) onRotate(e.delta);
        if (e.shortPress) onShortClick();
        if (e.longPress) onLongClick();
        if (e.veryLongPress) onVeryLongClick();
    }

    void tick() {
        const uint32_t now = millis();

        // 1Hz blink (fault emphasis)
        if (now - lastBlinkMs >= 500) {
            lastBlinkMs = now;
            blink = !blink;
        }

        // Sensor
        if (now - lastSensorMs >= cfg.sensorMs) {
            lastSensorMs = now;
            readEnv();
        }

        // Draw
        if (now - lastDrawMs < cfg.refreshMs) return;
        lastDrawMs = now;

        UiViewModel vm;
        vm.envValid = envValid;
        vm.tempC = tempC;
        vm.humPct = humPct;
        vm.st = motion->status();
        vm.cfg = motion->config();
        vm.screen = screen;
        vm.cursor = cursor;
        vm.page = page;
        vm.blink = blink;
        vm.uptimeMs = now;

        // Fault rendering policy
        if (vm.st.state == MotionState::Fault) {
            vm.invert = true;
            vm.blink = blink;
        }

        if (screen == UiScreen::EditValue) {
            vm.editLabel = editLabel;
            vm.editValue = editValue;
            vm.editMin = editMin;
            vm.editMax = editMax;
            vm.editUnit = editUnit;
        }

        renderer.draw(vm);
    }

private:
    UiConfig cfg;
    MotionController* motion = nullptr;
    UiRenderer_U8g2 renderer;

    // env
    Adafruit_AHTX0 aht;
    bool envOk = false;
    bool envValid = false;
    float tempC = 0;
    float humPct = 0;

    // ui state
    UiScreen screen = UiScreen::Main;
    uint8_t cursor = 0;
    uint8_t page = 0;

    // edit state
    enum class EditKind : uint8_t { None=0, MaxSpeed, Accel, Dwell, Rehome };
    EditKind editKind = EditKind::None;
    const char* editLabel = nullptr;
    const char* editUnit = nullptr;
    int32_t editValue = 0;
    int32_t editMin = 0;
    int32_t editMax = 0;
    UiScreen returnScreen = UiScreen::MenuParams;

    // timers
    uint32_t lastDrawMs = 0;
    uint32_t lastSensorMs = 0;
    uint32_t lastBlinkMs = 0;
    bool blink = false;

    static int32_t clampi(int32_t v, int32_t lo, int32_t hi) {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return v;
    }

    void readEnv() {
        if (!envOk) { envValid = false; return; }
        sensors_event_t hum, temp;
        if (aht.getEvent(&hum, &temp)) {
            tempC = temp.temperature;
            humPct = hum.relative_humidity;
            envValid = true;
        } else {
            envValid = false;
        }
    }

    void onRotate(int delta) {
        switch (screen) {
            case UiScreen::Main:
                page = (uint8_t)clampi((int32_t)page + delta, 0, 2); // 0..2
                break;
            case UiScreen::MenuRoot:
                cursor = (uint8_t)clampi((int32_t)cursor + delta, 0, 3);
                break;
            case UiScreen::MenuMotion:
                cursor = (uint8_t)clampi((int32_t)cursor + delta, 0, 3);
                break;
            case UiScreen::MenuParams:
                cursor = (uint8_t)clampi((int32_t)cursor + delta, 0, 3);
                break;
            case UiScreen::MenuDiag:
                page = (uint8_t)clampi((int32_t)page + delta, 0, 1);
                break;
            case UiScreen::MenuSystem:
                cursor = (uint8_t)clampi((int32_t)cursor + delta, 0, 1);
                break;
            case UiScreen::Engineering:
                cursor = (uint8_t)clampi((int32_t)cursor + delta, 0, 3);
                break;
            case UiScreen::EditValue:
                editValue = clampi(editValue + delta * editStep(), editMin, editMax);
                break;
        }
    }

    int32_t editStep() const {
        // Nice UX: parameter-specific step
        switch (editKind) {
            case EditKind::MaxSpeed: return 25;
            case EditKind::Accel:    return 50;
            case EditKind::Dwell:    return 25;
            case EditKind::Rehome:   return 1;
            default: break;
        }
        return 1;
    }

    void onShortClick() {
        switch (screen) {
            case UiScreen::Main:
                // Toggle Start/Stop
                if (motion->status().state == MotionState::Stopped) motion->requestStart();
                else motion->requestStop();
                break;
            case UiScreen::MenuRoot:
                enterFromRoot();
                break;
            case UiScreen::MenuMotion:
                selectMotion();
                break;
            case UiScreen::MenuParams:
                selectParam();
                break;
            case UiScreen::MenuDiag:
                // back
                screen = UiScreen::MenuRoot;
                cursor = 2;
                break;
            case UiScreen::MenuSystem:
                if (cursor == 1) { screen = UiScreen::MenuRoot; cursor = 3; }
                // reboot TODO
                break;
            case UiScreen::Engineering:
                selectEngineering();
                break;
            case UiScreen::EditValue:
                commitEdit();
                break;
        }
    }

    void onLongClick() {
        switch (screen) {
            case UiScreen::Main:
                screen = UiScreen::MenuRoot;
                cursor = 0;
                break;
            case UiScreen::MenuRoot:
                screen = UiScreen::Main;
                break;
            case UiScreen::MenuMotion:
            case UiScreen::MenuParams:
            case UiScreen::MenuDiag:
            case UiScreen::MenuSystem:
                screen = UiScreen::MenuRoot;
                cursor = 0;
                break;
            case UiScreen::Engineering:
                screen = UiScreen::MenuRoot;
                cursor = 0;
                break;
            case UiScreen::EditValue:
                // cancel
                screen = returnScreen;
                break;
        }
    }

    void onVeryLongClick() {
        // Hidden entry: from Main only
        if (screen == UiScreen::Main) {
            screen = UiScreen::Engineering;
            cursor = 0;
        }
    }

    void enterFromRoot() {
        switch (cursor) {
            case 0: screen = UiScreen::MenuMotion; cursor = 0; break;
            case 1: screen = UiScreen::MenuParams; cursor = 0; break;
            case 2: screen = UiScreen::MenuDiag;   cursor = 0; page = 0; break;
            case 3: screen = UiScreen::MenuSystem; cursor = 0; break;
        }
    }

    void selectMotion() {
        switch (cursor) {
            case 0: motion->requestStart(); break;
            case 1: motion->requestStop(); break;
            case 2: motion->requestHome(); break;
            case 3: motion->requestRecalibrate(); break;
        }
        screen = UiScreen::MenuRoot;
        cursor = 0;
    }

    void selectParam() {
        const auto& mc = motion->config();
        returnScreen = UiScreen::MenuParams;
        editUnit = "";
        editKind = EditKind::None;

        switch (cursor) {
            case 0:
                editKind = EditKind::MaxSpeed;
                editLabel = "최대 속도";
                editValue = (int32_t)mc.maxSps;
                editMin = 200;
                editMax = 2500;
                editUnit = "";
                break;
            case 1:
                editKind = EditKind::Accel;
                editLabel = "가속도";
                editValue = (int32_t)mc.accel;
                editMin = 100;
                editMax = 6000;
                editUnit = "";
                break;
            case 2:
                editKind = EditKind::Dwell;
                editLabel = "대기";
                editValue = (int32_t)mc.dwellMs;
                editMin = 0;
                editMax = 5000;
                editUnit = "ms";
                break;
            case 3:
                editKind = EditKind::Rehome;
                editLabel = "리홈 주기";
                editValue = (int32_t)mc.rehomeEveryCycles;
                editMin = 50;
                editMax = 500;
                editUnit = "";
                break;
        }

        screen = UiScreen::EditValue;
    }

    void commitEdit() {
        switch (editKind) {
            case EditKind::MaxSpeed: motion->requestSetMaxSps((float)editValue); break;
            case EditKind::Accel:    motion->requestSetAccel((float)editValue); break;
            case EditKind::Dwell:    motion->requestSetDwell((uint32_t)editValue); break;
            case EditKind::Rehome:   motion->requestSetRehomeEvery((uint32_t)editValue); break;
            default: break;
        }
        screen = returnScreen;
    }

    void selectEngineering() {
        // Engineering actions: keep them safe and reversible.
        switch (cursor) {
            case 0:
                motion->requestHome();
                break;
            case 1:
                motion->requestForceMoveLeft();
                break;
            case 2:
                motion->requestForceMoveRight();
                break;
            case 3:
                motion->requestDisableMotor();
                break;
        }
        screen = UiScreen::Main;
    }
};
