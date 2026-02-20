#pragma once
#include <Arduino.h>
#include "../../config/Defaults.h"
#include "../../config/PinMap.h"
#include "../../hal/StepperHal_Drv8825.h"

enum class MotionState : uint8_t {
    HomingLeft = 0,
    CalibMoveRight = 1,
    MoveLeft = 2,
    MoveRight = 3,
    Dwell = 4,
    Fault = 5,
    RecoverWait = 6,
    Stopped = 7
};

enum class MotionError : uint8_t {
    None = 0,
    HomingTimeout = 1,
    TravelTimeout = 2,
    CalibFailed = 3,
    BothLimitsActive = 4
};

struct MotionStatus {
    MotionState state = MotionState::HomingLeft;
    MotionError err = MotionError::None;
    float currentSps = 0;
    float targetSps = 0;
    long pos = 0;
    bool hallL = false;
    bool hallR = false;
    // raw digitalRead values (0/1) for diagnostics
    uint8_t hallRawL = 0;
    uint8_t hallRawR = 0;
    uint32_t travelSteps = 0;
    uint32_t cycles = 0;
    uint8_t recoverAttempts = 0;

    MotionError lastErr = MotionError::None;
    uint32_t faultTotal = 0;          // 누적 fault 횟수(전원 켠 동안)
    uint32_t lastFaultUptimeMs = 0;   // 마지막 fault 시각(ms)
    bool permanentFault = false;      // 3회 실패 후 유지 여부
};

class MotionController {
public:
    void begin(const MotionConfig& cfg_) {
        cfg = cfg_;
        pinMode(PIN_HALL_LEFT, INPUT_PULLDOWN);
        pinMode(PIN_HALL_RIGHT, INPUT_PULLDOWN);
        drv.begin();
        drv.enable(true);
        resetForHoming(true);
    }

    void applyConfig(const MotionConfig& cfg_) { cfg = cfg_; }

    const MotionConfig& config() const { return cfg; }

    const MotionStatus& status() const { return st; }

    // ---- UI-facing request API (non-blocking) ----
    void requestStart() { pending.start = true; }
    void requestStop()  { pending.stop = true; }
    void requestHome()  { pending.home = true; }
    void requestRecalibrate() { pending.recalibrate = true; }
    // Engineering
    void requestForceMoveLeft()  { pending.forceMoveLeft = true; }
    void requestForceMoveRight() { pending.forceMoveRight = true; }
    void requestDisableMotor()   { pending.stop = true; }
    void requestInjectFault(MotionError e) { pending.injectFault = true; pending.faultToInject = e; }
    void requestSetMaxSps(float sps) { pending.setMaxSps = true; pending.maxSps = sps; }
    void requestSetAccel(float a)    { pending.setAccel = true; pending.accel = a; }
    void requestSetDwell(uint32_t ms){ pending.setDwell = true; pending.dwellMs = ms; }
    void requestSetRehomeEvery(uint32_t cycles){ pending.setRehome = true; pending.rehomeEvery = cycles; }

    void tick() {
        const uint32_t nowMs = millis();
        const uint32_t nowUs = micros();

        // Apply pending parameter requests at the top of the tick.
        if (pending.setMaxSps) { cfg.maxSps = pending.maxSps; pending.setMaxSps = false; }
        if (pending.setAccel)  { cfg.accel  = pending.accel;  pending.setAccel  = false; }
        if (pending.setDwell)  { cfg.dwellMs= pending.dwellMs;pending.setDwell  = false; }
        if (pending.setRehome) { cfg.rehomeEveryCycles = pending.rehomeEvery; pending.setRehome = false; }

        // Command requests (start/stop/home/recalibrate)
        if (pending.stop) {
            pending.stop = false;
            enterStopped(nowMs);
        }

        if (pending.home || pending.recalibrate) {
            pending.home = false;
            pending.recalibrate = false;
            resetForHoming(true);  // 사용자 개입
        }

        if (pending.injectFault) {
            pending.injectFault = false;
            fault(pending.faultToInject);
            return;
        }

        if (pending.forceMoveLeft) {
            pending.forceMoveLeft = false;
            enterForcedMove(false);
        }
        if (pending.forceMoveRight) {
            pending.forceMoveRight = false;
            enterForcedMove(true);
        }

        if (pending.start) {
            pending.start = false;
            // If stopped, start by homing. Otherwise ignore (already running).
            if (st.state == MotionState::Stopped) resetForHoming(true);
        }

        st.hallRawL = (uint8_t)digitalRead(PIN_HALL_LEFT);
        st.hallRawR = (uint8_t)digitalRead(PIN_HALL_RIGHT);
        // Active-low hall modules: magnet close => LOW
        st.hallL = (st.hallRawL == HIGH);
        st.hallR = (st.hallRawR == HIGH);

        if (st.hallL && st.hallR) {
            fault(MotionError::BothLimitsActive);
            return;
        }

        if (st.state == MotionState::Stopped) {
            drv.enable(false);
            st.currentSps = 0;
            st.targetSps = 0;
            return;
        }

        if (st.state == MotionState::RecoverWait) {
            if (st.permanentFault) return;           // 🔒 영구 Fault면 자동 복구 금지
            if (nowMs - stateEnterMs >= 2000) resetForHoming(false);
            return;
        }

        if (st.state == MotionState::Fault) {
            drv.enable(false);

            if (st.recoverAttempts >= 3) {
                // 영구 Fault 유지 (사용자 개입 or 리셋까지)
                st.permanentFault = true;
                return;
            }

            // Fault 화면 유지 시간 후 RecoverWait로 이동
            if (nowMs - stateEnterMs >= 2000) {
                st.state = MotionState::RecoverWait;
                stateEnterMs = nowMs;
            }

            return;
        }

        if (st.state == MotionState::HomingLeft) {
            if (nowMs - stateEnterMs > cfg.homingTimeoutMs) {
                fault(MotionError::HomingTimeout);
                return;
            }
        } else if (st.state == MotionState::CalibMoveRight) {
            if (nowMs - stateEnterMs > cfg.travelTimeoutMs) {
                fault(MotionError::CalibFailed);
                return;
            }
        } else if (st.state == MotionState::MoveLeft || st.state == MotionState::MoveRight) {
            uint32_t limitMs = cfg.travelTimeoutMs;
            if (st.travelSteps > 0) {
                float t = (float)st.travelSteps / maxf(cfg.minSps, 1.0f);
                limitMs = (uint32_t)(t * 1000.0f) + 5000;
            }
            if (nowMs - stateEnterMs > limitMs) {
                fault(MotionError::TravelTimeout);
                return;
            }
        }

        switch (st.state) {
            case MotionState::HomingLeft:
                drv.enable(true);
                drv.setDir(false);
                st.targetSps = cfg.minSps;
                rampSpeed(nowMs, false);
                if (stepDue(nowUs)) {
                    doStep(false);
                }
                if (st.hallL) {
                    st.pos = 0;
                    st.currentSps = 0;
                    st.targetSps = cfg.minSps;
                    st.travelSteps = 0;
                    st.state = MotionState::CalibMoveRight;
                    stateEnterMs = nowMs;
                    lastStepUs = nowUs;
                    st.err = MotionError::None;
                    calibSteps = 0;
                    moveSteps = 0;
                }
                break;

            case MotionState::CalibMoveRight:
                drv.enable(true);
                drv.setDir(true);
                st.targetSps = cfg.maxSps;
                rampSpeed(nowMs, false);
                if (stepDue(nowUs)) {
                    doStep(true);
                    calibSteps++;
                }
                if (st.hallR) {
                    st.travelSteps = calibSteps;
                    calibSteps = 0;
                    enterDwell(nowMs, MotionState::MoveLeft);
                }
                break;

            case MotionState::MoveRight:
                drv.enable(true);
                drv.setDir(true);
                st.targetSps = cfg.maxSps;
                rampSpeed(nowMs, true);
                if (stepDue(nowUs)) {
                    doStep(true);
                    moveSteps++;
                }
                if (st.hallR) {
                    enterDwell(nowMs, MotionState::MoveLeft);
                }
                break;

            case MotionState::MoveLeft:
                drv.enable(true);
                drv.setDir(false);
                st.targetSps = cfg.maxSps;
                rampSpeed(nowMs, true);
                if (stepDue(nowUs)) {
                    doStep(false);
                    moveSteps++;
                }
                if (st.hallL) {
                    if (lastWasRightEnd) {
                        st.cycles++;
                        lastWasRightEnd = false;
                    }
                    enterDwell(nowMs, MotionState::MoveRight);
                }
                break;

            case MotionState::Dwell:
                drv.enable(false);
                st.currentSps = 0;
                if (nowMs - stateEnterMs >= cfg.dwellMs) {
                    if (st.cycles > 0 && (st.cycles % cfg.rehomeEveryCycles) == 0) {
                        resetForHoming(true);
                        return;
                    }
                    st.state = nextAfterDwell;
                    stateEnterMs = nowMs;
                    moveSteps = 0;
                    lastStepUs = nowUs;
                    if (nextAfterDwell == MotionState::MoveLeft) lastWasRightEnd = true;
                }
                break;

            default:
                break;
        }
    }

private:
    MotionConfig cfg;
    StepperHal_Drv8825 drv;
    MotionStatus st;

    uint32_t stateEnterMs = 0;
    uint32_t lastStepUs = 0;
    uint32_t lastRampMs = 0;

    uint32_t calibSteps = 0;
    uint32_t moveSteps = 0;
    bool lastWasRightEnd = false;

    MotionState nextAfterDwell = MotionState::MoveRight;

    struct Pending {
        bool start = false;
        bool stop = false;
        bool home = false;
        bool recalibrate = false;
        bool forceMoveLeft = false;
        bool forceMoveRight = false;
        bool injectFault = false;
        bool setMaxSps = false;
        bool setAccel = false;
        bool setDwell = false;
        bool setRehome = false;
        MotionError faultToInject = MotionError::None;
        float maxSps = 0;
        float accel = 0;
        uint32_t dwellMs = 0;
        uint32_t rehomeEvery = 0;
    } pending;

    static float maxf(float a, float b) { return a > b ? a : b; }

    void resetForHoming(bool userInitiated) {
        drv.enable(true);
        st.state = MotionState::HomingLeft;
        st.err = MotionError::None;
        st.currentSps = 0;
        st.targetSps = cfg.minSps;
        st.pos = 0;
        stateEnterMs = millis();
        lastStepUs = micros();
        lastRampMs = stateEnterMs;
        calibSteps = 0;
        moveSteps = 0;
        lastWasRightEnd = false;
        st.travelSteps = 0;

        if (userInitiated) {
            st.recoverAttempts = 0;
            st.permanentFault = false;
        }
    }

    void enterStopped(uint32_t nowMs) {
        st.state = MotionState::Stopped;
        st.currentSps = 0;
        st.targetSps = 0;
        drv.enable(false);
        stateEnterMs = nowMs;
    }

    void fault(MotionError e) {
        const uint32_t now = millis();

        // 최초 Fault 진입만 카운트 (Fault 상태에서 fault() 다시 호출되면 누적 방지)
        if (st.state != MotionState::Fault) {
            st.recoverAttempts++;             // retryCount (1..)
            st.faultTotal++;                  // 누적 fault 횟수
            st.lastErr = e;                   // 마지막 fault 기록
            st.lastFaultUptimeMs = now;       // timestamp
        }

        st.state = MotionState::Fault;
        st.err = e;
        st.currentSps = 0;
        st.targetSps = 0;
        drv.enable(false);
        stateEnterMs = now;

        // 3회 이상이면 영구 Fault 플래그
        st.permanentFault = (st.recoverAttempts >= 3);
    }

    void enterDwell(uint32_t nowMs, MotionState next) {
        st.state = MotionState::Dwell;
        nextAfterDwell = next;
        stateEnterMs = nowMs;
        st.currentSps = 0;
        st.targetSps = 0;
        drv.enable(false);
        moveSteps = 0;
    }

    void enterForcedMove(bool toRight) {
        // Engineering-only: move until a hall triggers or timeout.
        // We reuse MoveLeft/MoveRight states with travelSteps=0 so decel logic is disabled.
        drv.enable(true);
        st.err = MotionError::None;
        st.state = toRight ? MotionState::MoveRight : MotionState::MoveLeft;
        st.targetSps = cfg.minSps;
        st.currentSps = cfg.minSps;
        st.travelSteps = 0;
        moveSteps = 0;
        stateEnterMs = millis();
        lastStepUs = micros();
        lastRampMs = stateEnterMs;
    }

    bool stepDue(uint32_t nowUs) {
        float sps = maxf(st.currentSps, 1.0f);
        uint32_t intervalUs = (uint32_t)(1000000.0f / sps);
        return (uint32_t)(nowUs - lastStepUs) >= intervalUs;
    }

    void doStep(bool forward) {
        drv.stepPulse();
        lastStepUs = micros();
        st.pos += forward ? 1 : -1;
    }

    void rampSpeed(uint32_t nowMs, bool useDecel) {
        uint32_t dtMs = nowMs - lastRampMs;
        if (dtMs == 0) return;
        lastRampMs = nowMs;

        float dt = (float)dtMs / 1000.0f;
        float v = st.currentSps;
        float vmax = cfg.maxSps;
        float vmin = cfg.minSps;
        float a = maxf(cfg.accel, 1.0f);

        float desired = st.targetSps;

        if (useDecel && st.travelSteps > 0) {
            uint32_t traveled = moveSteps;
            if (traveled > st.travelSteps) traveled = st.travelSteps;
            uint32_t remaining = st.travelSteps - traveled;

            float brake = (v * v) / (2.0f * a);
            if ((float)remaining <= brake) desired = vmin;
            else desired = vmax;
        }

        if (v < desired) {
            v += a * dt;
            if (v > desired) v = desired;
        } else if (v > desired) {
            v -= a * dt;
            if (v < desired) v = desired;
        }

        if (v < vmin) v = vmin;
        if (v > vmax) v = vmax;
        st.currentSps = v;
    }
};
