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
    enum class EditKind : uint8_t { None=0, MaxSpeed, Accel, Dwell, Rehome, LedOnStart, LedOnEnd };
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

    // alert popup
    uint32_t lastAlertSeqSeen = 0;
    uint32_t popupSeq = 0;
    uint8_t  popupCode = 0;
    UiScreen popupReturn = UiScreen::Main;

    // toast (simple popup for non-fault notifications)
    UiScreen toastReturn = UiScreen::Main;
    uint8_t toastReturnCursor = 0;
    uint8_t toastReturnPage = 0;
    const char* toastTitle = nullptr;
    const char* toastLine1 = nullptr;
    const char* toastLine2 = nullptr;

    // test running
    enum class TestMode : uint8_t {
        None = 0,
        LedOn,
        LedOff,
        MoveLeft,
        MoveRight,
        TouchLeft,
        TouchRight,
        FactoryValidation
    };

    struct TestState {
        TestMode mode = TestMode::None;
        bool running = false;
        uint8_t step = 0;
        uint32_t startedMs = 0;
        uint32_t lastStepMs = 0;
    } test;

    // factory validation (motion sequence auto verification)
    enum class FactoryStep : uint8_t {
        Idle = 0,
        Start,
        WaitHoming,
        WaitCalib,
        WaitMoveLeft,
        WaitMoveRight,
        VerifyCycle,
        InjectFault,
        WaitFault,
        WaitRecoverWait,
        WaitRecoverHoming,
        WaitRecoverCalib,
        Complete,
        Failed
    };

    struct FactoryState {
        bool running = false;
        bool done = false;
        bool pass = false;
        FactoryStep step = FactoryStep::Idle;
        uint32_t stepStartMs = 0;
        uint8_t failCode = 0;
        uint8_t failStep = 0;
        uint32_t cycleStartCycles = 0;
        uint8_t injectedEnds = 0;
    } factory;

    static constexpr uint8_t PAGE_MAIN_MAX = 2; // 0..2
    static constexpr uint8_t PAGE_DIAG_MAX = 5; // 0..5 (Factory Log)

    static constexpr uint8_t ROOT_COUNT   = 5;
    static constexpr uint8_t MOTION_COUNT = 3;
    static constexpr uint8_t PARAM_COUNT  = 4;
    static constexpr uint8_t SYS_COUNT    = 4;
    static constexpr uint8_t LED_COUNT    = 5;
    static constexpr uint8_t TEST_COUNT   = 7;
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

    static const char* factoryStepName(FactoryStep s) {
        switch (s) {
            case FactoryStep::Start: return "Start";
            case FactoryStep::WaitHoming: return "Homing";
            case FactoryStep::WaitCalib: return "Calib";
            case FactoryStep::WaitMoveLeft: return "MoveL";
            case FactoryStep::WaitMoveRight: return "MoveR";
            case FactoryStep::VerifyCycle: return "Cycle";
            case FactoryStep::InjectFault: return "Inject";
            case FactoryStep::WaitFault: return "Fault";
            case FactoryStep::WaitRecoverWait: return "RecWait";
            case FactoryStep::WaitRecoverHoming: return "RecHome";
            case FactoryStep::WaitRecoverCalib: return "RecCalib";
            case FactoryStep::Complete: return "Complete";
            case FactoryStep::Failed: return "Failed";
            default: return "-";
        }
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
            case EditKind::LedOnStart: return 5;
            case EditKind::LedOnEnd:   return 5;
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
            case UiScreen::AlertPopup:
                if (motion) motion->acknowledgeAlert(popupSeq);
                lastAlertSeqSeen = popupSeq;
                gotoScreen(popupReturn, 0, page);
                break;

            case UiScreen::Toast:
                // no rotate
                break;

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
            case UiScreen::MenuLed:
                cursor = clampCursor((int32_t)cursor + delta, LED_COUNT);
                break;
            case UiScreen::MenuTest:
                cursor = clampCursor((int32_t)cursor + delta, TEST_COUNT);
                break;
            case UiScreen::Engineering:
                cursor = clampCursor((int32_t)cursor + delta, ENG_COUNT);
                break;
            case UiScreen::EditValue:
                editValue = clampi(editValue + (int32_t)delta * editStep(), editMin, editMax);
                break;
            case UiScreen::TestRunning:
                // no rotate
                break;
        }
    }

    void handleShortClick() {
        if (motion && motion->status().state == MotionState::Fault) {
            motion->requestHome();   // Retry
            return;
        }

        switch (screen) {
            case UiScreen::Toast:
                gotoScreen(toastReturn, toastReturnCursor, toastReturnPage);
                break;

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
                selectSystem();
                break;

            case UiScreen::MenuLed:
                selectLed();
                break;

            case UiScreen::MenuTest:
                selectTest();
                break;

            case UiScreen::TestRunning:
                // TestRunning controls:
                // - If factory finished, click returns to Test menu
                // - Otherwise, click = temporarily mute error popups (useful during scripted tests)
                if (factory.done) {
                    gotoScreen(UiScreen::MenuTest, 0, 0);
                } else if (motion) {
                    motion->setUiMuteSeconds(30);
                    showToast("TEST", "Mute Errors", "30s");
                }
                break;

            case UiScreen::TestResult:
                gotoScreen(UiScreen::MenuTest, 0, 0);
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
            case UiScreen::AlertPopup:
                // Jump to Recent Alerts page
                gotoScreen(UiScreen::MenuDiag, 0, 3);
                break;
            case UiScreen::Toast:
                gotoScreen(toastReturn, toastReturnCursor, toastReturnPage);
                break;
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
            case UiScreen::MenuLed:
            case UiScreen::MenuTest:
            case UiScreen::Engineering:
                gotoScreen(UiScreen::MenuRoot, 0, 0);
                break;
            case UiScreen::EditValue:
                // cancel
                gotoScreen(returnScreen, 0, 0);
                break;
            case UiScreen::TestRunning:
                if (test.mode == TestMode::FactoryValidation && factory.running) {
                    stopFactoryValidation(true, 250);
                    showToast("FACTORY", "Abort", "FAIL");
                }
                stopTest();
                gotoScreen(UiScreen::MenuTest, 0, 0);
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
            case 4: gotoScreen(UiScreen::MenuTest, 0, 0); break;
        }
    }

    void selectMotion() {
        if (!motion) return;

        switch (cursor) {
            case 0: motion->requestStart(); break;
            case 1: motion->requestStop(); break;
            case 2: motion->requestRecalibrate(); break;
        }

        // Policy: after action, return to root
        gotoScreen(UiScreen::MenuRoot, 0, 0);
    }

    void selectSystem() {
        // 0: Time Sync, 1: LED, 2: About (no-op), 3: Back
        if (cursor == 0) {
            // Time sync comes from LineBed (not implemented here yet)
            showToast("Time Sync", "LineBed sync", "(not connected)");
        } else if (cursor == 1) {
            gotoScreen(UiScreen::MenuLed, 0, 0);
        } else if (cursor == 3) {
            gotoScreen(UiScreen::MenuRoot, 3, 0);
        }
    }

    void showToast(const char* title, const char* l1, const char* l2) {
        toastTitle = title;
        toastLine1 = l1;
        toastLine2 = l2;
        toastReturn = screen;
        toastReturnCursor = cursor;
        toastReturnPage = page;
        gotoScreen(UiScreen::Toast, 0, 0);
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

        const auto& stNow = motion->status();

        switch (editKind) {
            case EditKind::MaxSpeed: motion->requestSetMaxSps((float)editValue); break;
            case EditKind::Accel:    motion->requestSetAccel((float)editValue); break;
            case EditKind::Dwell:    motion->requestSetDwell((uint32_t)editValue); break;
            case EditKind::Rehome:   motion->requestSetRehomeEvery((uint32_t)editValue); break;
            case EditKind::LedOnStart:
                motion->setLedModeAuto();
                motion->setLedScheduleMinutes((uint16_t)editValue, stNow.ledOnEndMin);
                break;
            case EditKind::LedOnEnd:
                motion->setLedModeAuto();
                motion->setLedScheduleMinutes(stNow.ledOnStartMin, (uint16_t)editValue);
                break;
            default: break;
        }

        // mark config dirty; actual flash write is debounced in main loop
        markPersistDirty();

        gotoScreen(returnScreen, 0, 0);
    }

    
    void selectLed() {
        if (!motion) return;

        // Menu items:
        // 0) Mode toggle (AUTO <-> MAN)
        // 1) Manual ON/OFF (forces Manual)
        // 2) Auto ON time (forces Auto)
        // 3) Auto OFF time (forces Auto)
        // 4) Back

        returnScreen = UiScreen::MenuLed;
        editUnit = "";
        editKind = EditKind::None;

        const auto& st = motion->status();

        // LED policy check: moving states must have LED ON (motor enable condition)
        if ((st.state == MotionState::HomingLeft || st.state == MotionState::CalibMoveRight ||
             st.state == MotionState::MoveLeft  || st.state == MotionState::MoveRight) &&
            !st.ledOn) {
            stopFactoryValidation(true, 201); // LED policy violation
            return;
        }

        switch (cursor) {
            case 0: {
                if (st.ledMode == LedMode::Auto) {
                    motion->setLedModeManual(st.ledManualOn);
                } else {
                    motion->setLedModeAuto();
                }
                // persist LED settings
                markPersistDirty();
                break;
            }
            case 1: {
                bool next = !st.ledManualOn;
                motion->setLedModeManual(next);
                markPersistDirty();

                if (next) motion->requestStart();   // ✅ ON이면 바로 구동
                break;
            }
            case 2: {
                editKind = EditKind::LedOnStart;
                editLabel = "LED ON 시작";
                editValue = (int32_t)st.ledOnStartMin;
                editMin = 0;
                editMax = 1439;
                editUnit = "";
                gotoScreen(UiScreen::EditValue, 0, 0);
                return;
            }
            case 3: {
                editKind = EditKind::LedOnEnd;
                editLabel = "LED OFF 종료";
                editValue = (int32_t)st.ledOnEndMin;
                editMin = 0;
                editMax = 1439;
                editUnit = "";
                gotoScreen(UiScreen::EditValue, 0, 0);
                return;
            }
            case 4: {
                gotoScreen(UiScreen::MenuSystem, 0, 0);
                return;
            }
        }
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

    void selectTest() {
        if (!motion) return;

        switch (cursor) {
            case 0: // LED ON
                motion->setLedModeManual(true);
                markPersistDirty();

                // ✅ LED ON은 모터 Enable 조건이므로,
                // "움직여야 한다" 정책이면 Start도 같이 요청해야 함
                motion->requestStart();

                showToast("TEST", "LED ON", "OK");
                break;
            case 1: // LED OFF
                motion->setLedModeManual(false);
                markPersistDirty();
                showToast("TEST", "LED OFF", "OK");
                break;
            case 2: // Move Left
                motion->requestForceMoveLeft();
                showToast("TEST", "Move Left", "Issued");
                break;
            case 3: // Move Right
                motion->requestForceMoveRight();
                showToast("TEST", "Move Right", "Issued");
                break;
            case 4: // Touch Left
                motion->requestSimulateHallLeft(300);
                showToast("TEST", "Touch Left", "Injected");
                break;
            case 5: // Touch Right
                motion->requestSimulateHallRight(300);
                showToast("TEST", "Touch Right", "Injected");
                break;
            case 6: // Factory Mode (10 cycles)
                if (!motion->isFactoryAutoTestRunning()) {

                    motion->startFactoryAutoTest(1000, 10);  // 1초 간격, 10 cycles

                    test.mode = TestMode::FactoryValidation;
                    test.running = true;
                    test.startedMs = millis();
                    test.lastStepMs = test.startedMs;

                    gotoScreen(UiScreen::TestRunning, 0, 0);
                } else {
                    motion->stopFactoryAutoTest();
                    stopTest();
                    gotoScreen(UiScreen::MenuTest, 0, 0);
                }
                break;
        }
    }

    void startFactoryValidation() {
        if (!motion) return;
        // Factory validation is a scripted test; suppress popups during the run.
        motion->setUiMuteSeconds(120);
        // reset previous results
        factory.running = true;
        factory.done = false;
        factory.pass = false;
        factory.failCode = 0;
        factory.step = FactoryStep::Start;
        factory.stepStartMs = millis();
        factory.cycleStartCycles = motion->status().cycles;
        factory.injectedEnds = 0;

        test.mode = TestMode::FactoryValidation;
        test.running = true;
        test.step = 0;
        test.startedMs = factory.stepStartMs;
        test.lastStepMs = factory.stepStartMs;
        gotoScreen(UiScreen::TestRunning, 0, 0);
    }

    void stopFactoryValidation(bool fail, uint8_t code) {
        factory.running = false;
        factory.done = true;
        factory.pass = !fail;
        factory.failCode = code;
        factory.failStep = (uint8_t)factory.step;
        factory.step = fail ? FactoryStep::Failed : FactoryStep::Complete;
        // keep TestRunning screen for result view
        test.running = false;

        // persist result into MotionController (so main.cpp can store it)
        if (motion) {
            uint32_t nowMs = millis();
            uint32_t durMs = nowMs - test.startedMs;
            motion->recordFactoryResult(!fail, code, factory.failStep, durMs, nowMs);
        }
    }

    void tickFactoryValidation(uint32_t now) {
        if (!factory.running) return;
        if (!motion) { stopFactoryValidation(true, 1); return; }

        const auto& st = motion->status();

        // LED policy check: moving states must have LED ON (motor enable condition)
        if ((st.state == MotionState::HomingLeft || st.state == MotionState::CalibMoveRight ||
             st.state == MotionState::MoveLeft  || st.state == MotionState::MoveRight) &&
            !st.ledOn) {
            stopFactoryValidation(true, 201); // LED policy violation
            return;
        }

        auto advance = [&](FactoryStep next) {
            factory.step = next;
            factory.stepStartMs = now;
            factory.injectedEnds = 0;
        };

        auto timeoutFail = [&](uint32_t ms, uint8_t code) {
            if (now - factory.stepStartMs > ms) {
                stopFactoryValidation(true, code);
            }
        };

        // IMPORTANT: We use simulated hall pulses to drive the state machine,
        // so this runs on bench without real sensors.
        switch (factory.step) {
            case FactoryStep::Start: {
                motion->requestStop();
                motion->requestStart();
                advance(FactoryStep::WaitHoming);
                break;
            }

            case FactoryStep::WaitHoming: {
                // Expect HomingLeft then move to CalibMoveRight
                if (st.state == MotionState::HomingLeft) {
                    if (!factory.injectedEnds && (now - factory.stepStartMs) > 600) {
                        motion->requestSimulateHallLeft(250);
                        factory.injectedEnds = 1;
                    }
                }
                if (st.state == MotionState::CalibMoveRight) {
                    advance(FactoryStep::WaitCalib);
                }
                if (st.state == MotionState::Fault) {
                    stopFactoryValidation(true, (uint8_t)st.err);
                }
                timeoutFail(8000, 2);
                break;
            }

            case FactoryStep::WaitCalib: {
                if (st.state == MotionState::CalibMoveRight) {
                    if (!factory.injectedEnds && (now - factory.stepStartMs) > 600) {
                        motion->requestSimulateHallRight(250);
                        factory.injectedEnds = 1;
                    }
                }
                // After right hit, it will Dwell then MoveLeft
                if (st.state == MotionState::MoveLeft) {
                    advance(FactoryStep::WaitMoveLeft);
                }
                if (st.state == MotionState::Fault) {
                    stopFactoryValidation(true, (uint8_t)st.err);
                }
                timeoutFail(10000, 3);
                break;
            }

            case FactoryStep::WaitMoveLeft: {
                if (st.state == MotionState::MoveLeft) {
                    if (!factory.injectedEnds && (now - factory.stepStartMs) > 800) {
                        motion->requestSimulateHallLeft(250);
                        factory.injectedEnds = 1;
                    }
                }
                if (st.state == MotionState::MoveRight) {
                    advance(FactoryStep::WaitMoveRight);
                }
                if (st.state == MotionState::Fault) {
                    stopFactoryValidation(true, (uint8_t)st.err);
                }
                timeoutFail(12000, 4);
                break;
            }

            case FactoryStep::WaitMoveRight: {
                if (st.state == MotionState::MoveRight) {
                    if (!factory.injectedEnds && (now - factory.stepStartMs) > 800) {
                        motion->requestSimulateHallRight(250);
                        factory.injectedEnds = 1;
                    }
                }
                // Require at least 1 cycle count increase
                if (st.cycles >= factory.cycleStartCycles + 1) {
                    advance(FactoryStep::VerifyCycle);
                }
                if (st.state == MotionState::Fault) {
                    stopFactoryValidation(true, (uint8_t)st.err);
                }
                timeoutFail(12000, 5);
                break;
            }

            case FactoryStep::VerifyCycle: {
                // Small settle time, then inject a fault to verify recovery.
                if (now - factory.stepStartMs > 500) {
                    advance(FactoryStep::InjectFault);
                }
                timeoutFail(3000, 6);
                break;
            }

            case FactoryStep::InjectFault: {
                motion->requestInjectFault(MotionError::TravelTimeout);
                advance(FactoryStep::WaitFault);
                break;
            }

            case FactoryStep::WaitFault: {
                if (st.state == MotionState::Fault) {
                    advance(FactoryStep::WaitRecoverWait);
                }
                timeoutFail(1200, 7);
                break;
            }

            case FactoryStep::WaitRecoverWait: {
                if (st.permanentFault) { stopFactoryValidation(true, 8); break; }
                if (st.state == MotionState::RecoverWait) {
                    advance(FactoryStep::WaitRecoverHoming);
                }
                timeoutFail(4500, 9);
                break;
            }

            case FactoryStep::WaitRecoverHoming: {
                if (st.state == MotionState::HomingLeft) {
                    if (!factory.injectedEnds && (now - factory.stepStartMs) > 600) {
                        motion->requestSimulateHallLeft(250);
                        factory.injectedEnds = 1;
                    }
                }
                if (st.state == MotionState::CalibMoveRight) {
                    advance(FactoryStep::WaitRecoverCalib);
                }
                timeoutFail(10000, 10);
                break;
            }

            case FactoryStep::WaitRecoverCalib: {
                if (st.state == MotionState::CalibMoveRight) {
                    if (!factory.injectedEnds && (now - factory.stepStartMs) > 600) {
                        motion->requestSimulateHallRight(250);
                        factory.injectedEnds = 1;
                    }
                }
                if (st.state == MotionState::MoveLeft || st.state == MotionState::MoveRight) {
                    stopFactoryValidation(false, 0);
                }
                if (st.state == MotionState::Fault) {
                    stopFactoryValidation(true, (uint8_t)st.err);
                }
                timeoutFail(12000, 11);
                break;
            }

            default:
                break;
        }
    }

    void stopTest() {
        test.running = false;
        test.mode = TestMode::None;
        test.step = 0;
        // do not auto-clear factory.done so the result can be shown
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

        // TestRunning tick
        if (screen == UiScreen::TestRunning) {
            if (test.mode == TestMode::FactoryValidation) {
                tickFactoryValidation(now);
            }

            if (test.mode == TestMode::FactoryValidation) {

                if (!motion->isFactoryAutoTestRunning()) {

                    // PASS 또는 FAIL 결과 화면으로 전환
                    gotoScreen(UiScreen::TestResult, 0, 0);
                    test.running = false;
                }
            }            
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

        // ---- Alert popup (non-fault) ----
        if (motion) {
            const auto& s = motion->status();
            const bool suppressPopups = factory.running || motion->isUiMuteActive();
            if (!suppressPopups && s.alertPending && s.alertSeq != 0 && s.state != MotionState::Fault) {
                if (s.alertSeq != lastAlertSeqSeen && screen != UiScreen::AlertPopup) {
                    popupSeq = s.alertSeq;
                    popupCode = s.alertPendingCode;
                    popupReturn = screen;
                    gotoScreen(UiScreen::AlertPopup, 0, 0);
                }
            }
        }

        if (motion) {
            const MotionStatus& st = motion->status();

            vm.factoryAutoRunning = motion->isFactoryAutoTestRunning();

            if (vm.factoryAutoRunning) {
                uint16_t progressed =
                    (uint16_t)(st.cycles - motion->factoryAutoStartCycles());
                vm.factoryAutoProgress = progressed;
                vm.factoryAutoTarget = motion->factoryAutoTargetCycles();
            } else {
                vm.factoryAutoProgress = 0;
                vm.factoryAutoTarget = 0;
            }
        }

        vm.screen = screen;
        vm.cursor = cursor;
        vm.page = page;
        vm.blink = blink;
        vm.uptimeMs = now;

        if (screen == UiScreen::Toast) {
            vm.showToast = true;
            vm.toastTitle = toastTitle;
            vm.toastLine1 = toastLine1;
            vm.toastLine2 = toastLine2;
        }

        if (screen == UiScreen::TestRunning) {
            vm.testRunning = test.running;
            vm.testStep = test.step;
            vm.testElapsedMs = now - test.startedMs;

            // factory validation view
            vm.factoryRunning = factory.running;
            vm.factoryDone = factory.done;
            vm.factoryPass = factory.pass;
            vm.factoryStep = (uint8_t)factory.step;
            vm.factoryStepName = factoryStepName(factory.step);
            vm.factoryFailCode = factory.failCode;
            vm.factoryFailStep = factory.failStep;
            vm.factoryStepElapsedMs = now - factory.stepStartMs;
        }

        if (screen == UiScreen::AlertPopup) {
            vm.showAlertPopup = true;
            vm.popupFaultCode = popupCode;
            vm.popupAlertSeq = popupSeq;
        }

        // --- HARD FAULT OVERRIDE (industrial) ---
        // During scripted tests (Factory/TestRunning) we keep the UI on TestRunning to avoid interruption.
        const bool suppressFaultOverlay = (screen == UiScreen::TestRunning) && (factory.running || (motion && motion->isUiMuteActive()));
        if (motion && motion->status().state == MotionState::Fault && !suppressFaultOverlay) {
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
            vm.editAsTime = (editKind == EditKind::LedOnStart || editKind == EditKind::LedOnEnd);
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
            case 5:
                vm.faultTitle = "Motion Stall";
                vm.faultDetail = "No pulse / no end hit";
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
