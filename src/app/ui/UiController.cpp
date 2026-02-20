#include "UiController.h"

#include <Wire.h>
#include <Adafruit_AHTX0.h>

#include "../../config/PinMap.h"
#include "../../config/Defaults.h"
#include "../controllers/MotionController.h"
#include "UiRenderer_U8g2.h"

// defined in main.cpp
extern void markPersistDirty();

// -----------------------------
// Internal implementation
// -----------------------------

struct UiController::Impl {
    UiConfig cfg{};
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

    static constexpr uint8_t PAGE_MAIN_MAX = 2; // 0..2
    static constexpr uint8_t PAGE_DIAG_MAX = 2; // 0..2

    static constexpr uint8_t ROOT_COUNT   = 4;
    static constexpr uint8_t MOTION_COUNT = 4;
    static constexpr uint8_t PARAM_COUNT  = 4;
    static constexpr uint8_t SYS_COUNT    = 2;
    static constexpr uint8_t ENG_COUNT    = 4;

    // ---- helpers ----
    static int32_t clampi(int32_t v, int32_t lo, int32_t hi) {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return v;
    }

    static uint8_t clampCursor(int32_t v, uint8_t count) {
        if (count == 0) return 0;
        if (v < 0) return 0;
        if (v >= (int32_t)count) return (uint8_t)(count - 1);
        return (uint8_t)v;
    }

    void gotoScreen(UiScreen s, uint8_t c = 0, uint8_t p = 0) {
        screen = s;
        cursor = c;
        page = p;
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

    int32_t editStep() const {
        // UX: parameter-specific step
        switch (editKind) {
            case EditKind::MaxSpeed: return 25;
            case EditKind::Accel:    return 50;
            case EditKind::Dwell:    return 25;
            case EditKind::Rehome:   return 1;
            default: break;
        }
        return 1;
    }

    // -----------------------------
    // UI event dispatch
    // -----------------------------

    void handleRotate(int8_t delta) {
        if (delta == 0) return;

        switch (screen) {
            case UiScreen::Main:
                page = clampCursor((int32_t)page + delta, PAGE_MAIN_MAX + 1);
                break;
            case UiScreen::MenuRoot:
                cursor = clampCursor((int32_t)cursor + delta, ROOT_COUNT);
                break;
            case UiScreen::MenuMotion:
                cursor = clampCursor((int32_t)cursor + delta, MOTION_COUNT);
                break;
            case UiScreen::MenuParams:
                cursor = clampCursor((int32_t)cursor + delta, PARAM_COUNT);
                break;
            case UiScreen::MenuDiag:
                page = clampCursor((int32_t)page + delta, PAGE_DIAG_MAX + 1);
                break;
            case UiScreen::MenuSystem:
                cursor = clampCursor((int32_t)cursor + delta, SYS_COUNT);
                break;
            case UiScreen::Engineering:
                cursor = clampCursor((int32_t)cursor + delta, ENG_COUNT);
                break;
            case UiScreen::EditValue:
                editValue = clampi(editValue + (int32_t)delta * editStep(), editMin, editMax);
                break;
        }
    }

    void handleShortClick() {
        if (motion && motion->status().state == MotionState::Fault) {
            motion->requestHome();   // Retry
            return;
        }

        switch (screen) {
            case UiScreen::Main:
                // Toggle Start/Stop
                if (!motion) break;
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
                // short click = back (simple)
                gotoScreen(UiScreen::MenuRoot, 2, 0);
                break;

            case UiScreen::MenuSystem:
                // cursor 0: about (no-op), cursor 1: back
                if (cursor == 1) gotoScreen(UiScreen::MenuRoot, 3, 0);
                break;

            case UiScreen::Engineering:
                selectEngineering();
                break;

            case UiScreen::EditValue:
                commitEdit();
                break;
        }
    }

    void handleLongClick() {
        if (motion && motion->status().state == MotionState::Fault) {
            rp2040.reboot();   // RP2040 리셋
            return;
        }

        switch (screen) {
            case UiScreen::Main:
                gotoScreen(UiScreen::MenuRoot, 0, 0);
                break;
            case UiScreen::MenuRoot:
                gotoScreen(UiScreen::Main, 0, page);
                break;
            case UiScreen::MenuMotion:
            case UiScreen::MenuParams:
            case UiScreen::MenuDiag:
            case UiScreen::MenuSystem:
            case UiScreen::Engineering:
                gotoScreen(UiScreen::MenuRoot, 0, 0);
                break;
            case UiScreen::EditValue:
                // cancel
                gotoScreen(returnScreen, 0, 0);
                break;
        }
    }

    void handleVeryLongClick() {
        // Hidden entry: from Main only
        if (screen == UiScreen::Main) {
            gotoScreen(UiScreen::Engineering, 0, 0);
        }
    }

    // -----------------------------
    // Menu actions
    // -----------------------------

    void enterFromRoot() {
        switch (cursor) {
            case 0: gotoScreen(UiScreen::MenuMotion, 0, 0); break;
            case 1: gotoScreen(UiScreen::MenuParams, 0, 0); break;
            case 2: gotoScreen(UiScreen::MenuDiag,   0, 0); break;
            case 3: gotoScreen(UiScreen::MenuSystem, 0, 0); break;
        }
    }

    void selectMotion() {
        if (!motion) return;

        switch (cursor) {
            case 0: motion->requestStart(); break;
            case 1: motion->requestStop(); break;
            case 2: motion->requestHome(); break;
            case 3: motion->requestRecalibrate(); break;
        }

        // Policy: after action, return to root
        gotoScreen(UiScreen::MenuRoot, 0, 0);
    }

    void selectParam() {
        if (!motion) return;

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

        gotoScreen(UiScreen::EditValue, 0, 0);
    }

    void commitEdit() {
        if (!motion) return;

        switch (editKind) {
            case EditKind::MaxSpeed: motion->requestSetMaxSps((float)editValue); break;
            case EditKind::Accel:    motion->requestSetAccel((float)editValue); break;
            case EditKind::Dwell:    motion->requestSetDwell((uint32_t)editValue); break;
            case EditKind::Rehome:   motion->requestSetRehomeEvery((uint32_t)editValue); break;
            default: break;
        }

        // mark config dirty; actual flash write is debounced in main loop
        markPersistDirty();

        gotoScreen(returnScreen, 0, 0);
    }

    void selectEngineering() {
        if (!motion) return;

        // Engineering actions: keep them safe and reversible.
        switch (cursor) {
            case 0: motion->requestHome(); break;
            case 1: motion->requestForceMoveLeft(); break;
            case 2: motion->requestForceMoveRight(); break;
            case 3: motion->requestDisableMotor(); break;
        }

        // Policy: after engineering action, go back to Main (safer)
        gotoScreen(UiScreen::Main, 0, page);
    }

    // -----------------------------
    // main tick
    // -----------------------------

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
        if (motion) {
            vm.st = motion->status();
            vm.cfg = motion->config();

            const auto& s = motion->status();
            vm.faultTotal = s.faultTotal;
            vm.lastFaultUptimeMs = s.lastFaultUptimeMs;
            vm.lastFaultCode = static_cast<uint8_t>(s.lastErr);
            vm.permanentFault = s.permanentFault;
        }
        vm.screen = screen;
        vm.cursor = cursor;
        vm.page = page;
        vm.blink = blink;
        vm.uptimeMs = now;

        // --- HARD FAULT OVERRIDE (industrial) ---
        if (motion && motion->status().state == MotionState::Fault) {
            vm.isFault = true;
            vm.faultCode = static_cast<uint8_t>(motion->status().err);
            vm.retryCount = motion->status().recoverAttempts;
            mapFault(vm);
            renderer.draw(vm);
            return; // 🔥 IMPORTANT: ignore other UI screens while fault
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

    void mapFault(UiViewModel& vm) {
        switch (vm.faultCode) {
            case 1:
                vm.faultTitle = "Homing Timeout";
                vm.faultDetail = "Left sensor not detected";
                break;
            case 2:
                vm.faultTitle = "Travel Timeout";
                vm.faultDetail = "Target not reached";
                break;
            case 3:
                vm.faultTitle = "Calibration Fail";
                vm.faultDetail = "Right sensor failed";
                break;
            case 4:
                vm.faultTitle = "Both Limits";
                vm.faultDetail = "Left & Right active";
                break;
            default:
                vm.faultTitle = "Unknown Fault";
                vm.faultDetail = "";
                break;
        }
    }
};

// -----------------------------
// UiController public wrapper
// -----------------------------

UiController::UiController() : _(new Impl()) {}
UiController::~UiController() { delete _; }

void UiController::begin(const UiConfig& cfg, MotionController* motion) {
    _->cfg = cfg;
    _->motion = motion;

    Wire.setSDA(PIN_I2C_SDA);
    Wire.setSCL(PIN_I2C_SCL);
    Wire.begin();

    _->renderer.begin();

    // AHT10/20
    _->envOk = _->aht.begin(&Wire);
    _->lastSensorMs = 0;
    _->lastDrawMs = 0;
    _->lastBlinkMs = 0;
    _->blink = false;
}

void UiController::handleEncoder(const EncoderEvents& e) {
    if (e.delta != 0) _->handleRotate((int8_t)e.delta);
    // Prefer very-long over long (Engineering must not be pre-empted by Menu long-click)
    if (e.veryLongPress) { _->handleVeryLongClick(); return; }
    if (e.shortPress) _->handleShortClick();
    if (e.longPress) _->handleLongClick();
}

void UiController::tick() {
    _->tick();
}
