#include "MotionController.h"

// ---- static helpers ----
float MotionController::maxf(float a, float b) { return a > b ? a : b; }

// ---- UI mute ----
void MotionController::setUiMuteSeconds(uint16_t seconds) {
    uiMuteUntilMs = millis() + (uint32_t)seconds * 1000UL;
}
bool MotionController::isUiMuteActive() const {
    return (uiMuteUntilMs != 0) && ((int32_t)(millis() - uiMuteUntilMs) < 0);
}

// ---- lifecycle ----
void MotionController::begin(const MotionConfig& cfg_) {
    cfg = cfg_;

    pinMode(PIN_HALL_LEFT, INPUT_PULLDOWN);
    pinMode(PIN_HALL_RIGHT, INPUT_PULLDOWN);

    pinMode(PIN_GROW_LED, OUTPUT);
    digitalWrite(PIN_GROW_LED, LOW);

    drv.begin();
    drv.enable(false); // LED policy decides

    resetForHoming(true);
}

// ---- persist restore ----
void MotionController::applyPersistedAlerts(uint32_t seq, uint8_t head, uint8_t count,
                                            const uint8_t* codes, const uint32_t* uptimeSec) {
    alerts.seq = seq;
    alerts.head = head % 5;
    alerts.count = (count > 5) ? 5 : count;

    for (uint8_t i = 0; i < 5; i++) {
        alerts.codes[i] = codes ? codes[i] : 0;
        alerts.uptimeSec[i] = uptimeSec ? uptimeSec[i] : 0;
    }

    syncAlertStatus();
}

void MotionController::applyPersistedFactory(uint32_t seq, bool lastPass, uint8_t failCode, uint8_t failStep,
                                             uint32_t durationMs, uint32_t uptimeSec,
                                             uint32_t passCount, uint32_t failCount,
                                             uint8_t logHead, uint8_t logCount,
                                             const uint8_t* logPass, const uint8_t* logFailCode, const uint8_t* logFailStep,
                                             const uint16_t* logDurationSec, const uint32_t* logUptimeSec, const uint32_t* logCycles) {
    factory.seq = seq;
    factory.lastPass = lastPass;
    factory.failCode = failCode;
    factory.failStep = failStep;
    factory.durationMs = durationMs;
    factory.uptimeSec = uptimeSec;
    factory.passCount = passCount;
    factory.failCount = failCount;

    factory.logHead = logHead % 8;
    factory.logCount = (logCount > 8) ? 8 : logCount;

    for (uint8_t i = 0; i < 8; i++) {
        factory.logPass[i] = logPass ? logPass[i] : 0;
        factory.logFailCode[i] = logFailCode ? logFailCode[i] : 0;
        factory.logFailStep[i] = logFailStep ? logFailStep[i] : 0;
        factory.logDurationSec[i] = logDurationSec ? logDurationSec[i] : 0;
        factory.logUptimeSec[i] = logUptimeSec ? logUptimeSec[i] : 0;
        factory.logCycles[i] = logCycles ? logCycles[i] : 0;
    }

    syncFactoryStatus();
}

// ---- factory result record ----
void MotionController::recordFactoryResult(bool pass, uint8_t failCode, uint8_t failStep,
                                           uint32_t durationMs, uint32_t uptimeMs) {
    factory.seq++;
    factory.lastPass = pass;
    factory.failCode = failCode;
    factory.failStep = failStep;
    factory.durationMs = durationMs;
    factory.uptimeSec = uptimeMs / 1000;
    if (pass) factory.passCount++;
    else factory.failCount++;

    pushFactoryLog(pass, failCode, failStep, durationMs, uptimeMs, st.cycles);

    if (factory.cb) {
        factory.cb(factory.seq, pass, failCode, failStep, durationMs, uptimeMs, st.cycles);
    }

    syncFactoryStatus();
}

// ---- callbacks ----
void MotionController::setAlertCallback(AlertCallback cb) { alerts.cb = cb; }
void MotionController::setFactoryCallback(FactoryCallback cb) { factory.cb = cb; }

// ---- alert ack ----
void MotionController::acknowledgeAlert(uint32_t seq) {
    if (alerts.seq == seq) {
        alerts.pending = false;
        alerts.pendingCode = 0;
        syncAlertStatus();
    }
}

// ---- config ----
void MotionController::applyConfig(const MotionConfig& cfg_) { cfg = cfg_; }
const MotionConfig& MotionController::config() const { return cfg; }
const MotionStatus& MotionController::status() const { return st; }

// ---- LED API ----
void MotionController::setLedModeAuto() { led.mode = LedMode::Auto; }
void MotionController::setLedModeManual(bool on) { led.mode = LedMode::Manual; led.manualOn = on; }
void MotionController::setLedScheduleMinutes(uint16_t onStartMin, uint16_t onEndMin) {
    led.onStartMin = onStartMin;
    led.onEndMin = onEndMin;
}
void MotionController::setClockMinutes(uint16_t minutesSinceMidnight) {
    led.clockValid = true;
    led.clockMin = minutesSinceMidnight % 1440;
}

// ---- safety API ----
void MotionController::setMotionStallPulseTimeoutMs(uint32_t ms) { safety.pulseStallTimeoutMs = ms; }
void MotionController::setMotionStallNoEndTimeoutMs(uint32_t ms) { safety.noEndTimeoutMs = ms; }

// ---- request API ----
void MotionController::requestStart() { pending.start = true; }
void MotionController::requestStop() { pending.stop = true; }
void MotionController::requestHome() { pending.home = true; }
void MotionController::requestRecalibrate() { pending.recalibrate = true; }
void MotionController::requestForceMoveLeft() { pending.forceMoveLeft = true; }
void MotionController::requestForceMoveRight() { pending.forceMoveRight = true; }
void MotionController::requestDisableMotor() { pending.stop = true; }
void MotionController::requestInjectFault(MotionError e) { pending.injectFault = true; pending.faultToInject = e; }
void MotionController::requestSetMaxSps(float sps) { pending.setMaxSps = true; pending.maxSps = sps; }
void MotionController::requestSetAccel(float a) { pending.setAccel = true; pending.accel = a; }
void MotionController::requestSetDwell(uint32_t ms) { pending.setDwell = true; pending.dwellMs = ms; }
void MotionController::requestSetRehomeEvery(uint32_t cycles) { pending.setRehome = true; pending.rehomeEvery = cycles; }

// ---- simulate hall ----
void MotionController::requestSimulateHallLeft(uint16_t activeMs) {
    sim.leftActive = true;
    sim.leftUntilMs = millis() + activeMs;
}
void MotionController::requestSimulateHallRight(uint16_t activeMs) {
    sim.rightActive = true;
    sim.rightUntilMs = millis() + activeMs;
}

// ---- tick ----
void MotionController::tick() {
    const uint32_t nowMs = millis();
    const uint32_t nowUs = micros();

    // --- Auto Hall Toggle (test mode) ---
    // This runs BEFORE reading real pins, so simulation can drive the FSM.
    if (autoHall.enabled) {
        if ((uint32_t)(nowMs - autoHall.lastToggleMs) >= autoHall.intervalMs) {
            autoHall.lastToggleMs = nowMs;

            if (autoHall.nextLeft) requestSimulateHallLeft((uint16_t)autoHall.pulseMs);
            else                  requestSimulateHallRight((uint16_t)autoHall.pulseMs);

            autoHall.nextLeft = !autoHall.nextLeft;
        }
    }    

    // keep status fields in sync for UI
    syncAlertStatus();
    syncFactoryStatus();

    // --- LED policy evaluation (master switch) ---
    const bool ledShouldBeOn = evalLedShouldBeOn();
    applyLedAndMotorPolicy(ledShouldBeOn);

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

    // --- Test simulation expire (for UI/Test menu) ---
    if (sim.leftActive && nowMs >= sim.leftUntilMs) sim.leftActive = false;
    if (sim.rightActive && nowMs >= sim.rightUntilMs) sim.rightActive = false;

    st.hallRawL = (uint8_t)digitalRead(PIN_HALL_LEFT);
    st.hallRawR = (uint8_t)digitalRead(PIN_HALL_RIGHT);

    // Hall polarity is configurable via HALL_ACTIVE_LOW.
    bool hallL = HALL_ACTIVE_LOW ? (st.hallRawL == LOW) : (st.hallRawL == HIGH);
    bool hallR = HALL_ACTIVE_LOW ? (st.hallRawR == LOW) : (st.hallRawR == HIGH);

    // Simulation overrides
    if (sim.leftActive)  hallL = true;
    if (sim.rightActive) hallR = true;

    st.hallL = hallL;
    st.hallR = hallR;

    // Update hall edge timestamps (used for stall detection)
    updateHallHealth(nowMs);

    // Both active simultaneously is abnormal; debounce to ignore glitches.
    if (st.hallL && st.hallR) {
        if (bothActiveSinceMs == 0) bothActiveSinceMs = nowMs;
        if ((nowMs - bothActiveSinceMs) >= safety.bothActiveDebounceMs) {
            fault(MotionError::BothLimitsActive);
            return;
        }
    } else {
        bothActiveSinceMs = 0;
    }

    // If LED policy says OFF, motor is forced disabled and we keep Stopped.
    if (!ledShouldBeOn) {
        if (st.state != MotionState::Stopped) enterStopped(nowMs);
        return;
    }

    if (st.state == MotionState::Stopped) {
        drv.enable(false);
        st.currentSps = 0;
        st.targetSps = 0;
        return;
    }

    // --- Safety: stall detection while LED is ON (motor expected to be alive) ---
    // 1) Pulse-level stall: no step pulse for too long while in a moving state
    if (isMovingState(st.state) && (nowMs - safety.lastStepPulseMs) > safety.pulseStallTimeoutMs) {
        fault(MotionError::MotionStall);
        return;
    }

    // 2) End-sensor liveness: no end hit for too long while system is active
    uint32_t noEndLimit = safety.noEndTimeoutMs;
    if (noEndLimit == 0) {
        // Auto-derive a conservative limit from current learned travel + dwell.
        noEndLimit = deriveNoEndTimeoutMs();
    }
    if ((nowMs - safety.lastEndHitMs) > noEndLimit) {
        fault(MotionError::MotionStall);
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

    // --- Factory AutoTest judge (non-invasive) ---
    if (fauto.running) {
        // FAIL 조건: Fault 상태 진입
        if (st.state == MotionState::Fault) {
            fauto.running = false;
            const uint32_t durMs = millis() - fauto.startMs;
            // MotionError를 failCode로 기록
            recordFactoryResult(false, (uint8_t)st.err, fauto.failStep, durMs, millis());
        } else {
            // PASS 조건: 목표 cycles 달성
            const uint16_t progressed = (uint16_t)(st.cycles - fauto.startCycles);
            if (progressed >= fauto.targetCycles) {
                fauto.running = false;
                const uint32_t durMs = millis() - fauto.startMs;
                recordFactoryResult(true, 0, fauto.failStep, durMs, millis());
            }
        }
    }

    switch (st.state) {
        case MotionState::HomingLeft:
            drv.enable(true);
            drv.setDir(false);
            st.targetSps = cfg.minSps;
            rampSpeed(nowMs, false);
            if (stepDue(nowUs)) {
                doStep(false, nowMs, nowUs);
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
                doStep(true, nowMs, nowUs);
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
                doStep(true, nowMs, nowUs);
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
                doStep(false, nowMs, nowUs);
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

// ---- internal helpers ----
void MotionController::resetForHoming(bool userInitiated) {
    drv.enable(true);
    st.state = MotionState::HomingLeft;
    st.err = MotionError::None;
    st.currentSps = 0;
    st.targetSps = cfg.minSps;
    st.pos = 0;
    stateEnterMs = millis();
    lastStepUs = micros();
    lastRampMs = stateEnterMs;
    safety.lastStepPulseMs = stateEnterMs;
    // don't reset lastEndHitMs here; we want to detect "no end hit since LED on".
    calibSteps = 0;
    moveSteps = 0;
    lastWasRightEnd = false;
    st.travelSteps = 0;

    if (userInitiated) {
        st.recoverAttempts = 0;
        st.permanentFault = false;
    }
}

//-----------------------------------------------
// BEGIN:: Auto Test, FactoryAutoTest
//-----------------------------------------------
void MotionController::setAutoHallTest(bool enabled) {
    autoHall.enabled = enabled;
    autoHall.lastToggleMs = millis();
    autoHall.nextLeft = true;
}

bool MotionController::isAutoHallTestEnabled() const {
    return autoHall.enabled;
}

void MotionController::setAutoHallIntervalMs(uint32_t intervalMs) {
    // clamp (너무 짧으면 의미없고, 너무 길면 테스트 지루)
    if (intervalMs < 300) intervalMs = 300;
    if (intervalMs > 30000) intervalMs = 30000;
    autoHall.intervalMs = intervalMs;
}

uint32_t MotionController::autoHallIntervalMs() const {
    return autoHall.intervalMs;
}

void MotionController::startFactoryAutoTest(uint32_t hallIntervalMs, uint16_t targetCycles) {

    if (targetCycles == 0) targetCycles = 10;

    fauto.running = true;
    fauto.targetCycles = targetCycles;
    fauto.startCycles = (uint16_t)st.cycles;
    fauto.startMs = millis();
    fauto.failStep = 1;

    // UI 알림 억제 (테스트 중 방해 방지)
    setUiMuteSeconds(60);

    // 테스트 조건 강제
    setLedModeManual(true);
    autoHall.intervalMs = hallIntervalMs;
    autoHall.enabled = true;
    autoHall.lastToggleMs = millis();
    autoHall.nextLeft = true;
    
    requestStart();
}

void MotionController::stopFactoryAutoTest() {
    fauto.running = false;

    // 🔥 반드시 AutoHall 끈다
    autoHall.enabled = false;
}

bool MotionController::isFactoryAutoTestRunning() const { return fauto.running; }
uint16_t MotionController::factoryAutoTargetCycles() const { return fauto.targetCycles; }
uint16_t MotionController::factoryAutoStartCycles() const { return fauto.startCycles; }
// END:: Auto Test, FactoryAutoTest
//-----------------------------------------------

void MotionController::enterStopped(uint32_t nowMs) {
    st.state = MotionState::Stopped;
    st.currentSps = 0;
    st.targetSps = 0;
    drv.enable(false);
    stateEnterMs = nowMs;
}

void MotionController::fault(MotionError e) {
    const uint32_t now = millis();

    // 최초 Fault 진입만 카운트 (Fault 상태에서 fault() 다시 호출되면 누적 방지)
    if (st.state != MotionState::Fault) {
        st.recoverAttempts++;             // retryCount (1..)
        st.faultTotal++;                  // 누적 fault 횟수
        st.lastErr = e;                   // 마지막 fault 기록
        st.lastFaultUptimeMs = now;       // timestamp

        // ---- alert ring buffer + callback (LineBed EVT) ----
        // During UI/Test mute window we still record the ring, but we don't raise a pending popup.
        const uint32_t upSec = now / 1000;
        alerts.codes[alerts.head] = (uint8_t)e;
        alerts.uptimeSec[alerts.head] = upSec;
        alerts.head = (uint8_t)((alerts.head + 1) % 5);
        if (alerts.count < 5) alerts.count++;
        alerts.seq++;

        if (!isUiMuteActive()) {
            alerts.pending = true;
            alerts.pendingCode = (uint8_t)e;
        }

        if (alerts.cb) {
            // callback is useful for logging even during mute
            alerts.cb((uint8_t)e, alerts.seq, now, st.cycles);
        }
    }

    st.state = MotionState::Fault;
    st.err = e;
    st.currentSps = 0;
    st.targetSps = 0;
    drv.enable(false);
    stateEnterMs = now;

    // 3회 이상이면 영구 Fault 플래그
    st.permanentFault = (st.recoverAttempts >= 3);

    syncAlertStatus();
}

void MotionController::syncFactoryStatus() {
    st.factorySeq = factory.seq;
    st.factoryLastPass = factory.lastPass;
    st.factoryFailCode = factory.failCode;
    st.factoryFailStep = factory.failStep;
    st.factoryLastDurationMs = factory.durationMs;
    st.factoryLastUptimeSec = factory.uptimeSec;
    st.factoryPassCount = factory.passCount;
    st.factoryFailCount = factory.failCount;

    st.factoryLogHead = factory.logHead;
    st.factoryLogCount = factory.logCount;

    for (uint8_t i = 0; i < 8; i++) {
        st.factoryLogPass[i] = factory.logPass[i];
        st.factoryLogFailCode[i] = factory.logFailCode[i];
        st.factoryLogFailStep[i] = factory.logFailStep[i];
        st.factoryLogDurationSec[i] = factory.logDurationSec[i];
        st.factoryLogUptimeSec[i] = factory.logUptimeSec[i];
        st.factoryLogCycles[i] = factory.logCycles[i];
    }
}

void MotionController::pushFactoryLog(bool pass, uint8_t failCode, uint8_t failStep,
                                      uint32_t durationMs, uint32_t uptimeMs, uint32_t cycles) {
    const uint8_t idx = factory.logHead % 8;

    factory.logPass[idx] = pass ? 1 : 0;
    factory.logFailCode[idx] = failCode;
    factory.logFailStep[idx] = failStep;
    factory.logDurationSec[idx] = (uint16_t)((durationMs + 500) / 1000);
    factory.logUptimeSec[idx] = uptimeMs / 1000;
    factory.logCycles[idx] = cycles;

    factory.logHead = (uint8_t)((factory.logHead + 1) % 8);
    if (factory.logCount < 8) factory.logCount++;
}

void MotionController::syncAlertStatus() {
    st.alertSeq = alerts.seq;
    st.alertHead = alerts.head;
    st.alertCount = alerts.count;

    for (uint8_t i = 0; i < 5; i++) {
        st.alertCodes[i] = alerts.codes[i];
        st.alertUptimeSec[i] = alerts.uptimeSec[i];
    }

    st.alertPending = alerts.pending;
    st.alertPendingCode = alerts.pendingCode;
}

void MotionController::enterDwell(uint32_t nowMs, MotionState next) {
    st.state = MotionState::Dwell;
    nextAfterDwell = next;
    stateEnterMs = nowMs;
    st.currentSps = 0;
    st.targetSps = 0;
    drv.enable(false);
    moveSteps = 0;
}

void MotionController::enterForcedMove(bool toRight) {
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
    safety.lastStepPulseMs = stateEnterMs;
}

bool MotionController::stepDue(uint32_t nowUs) {
    float sps = maxf(st.currentSps, 1.0f);
    uint32_t intervalUs = (uint32_t)(1000000.0f / sps);
    return (uint32_t)(nowUs - lastStepUs) >= intervalUs;
}

void MotionController::doStep(bool forward, uint32_t nowMs, uint32_t nowUs) {
    drv.stepPulse();
    lastStepUs = nowUs;
    safety.lastStepPulseMs = nowMs;
    st.pos += forward ? 1 : -1;
}

bool MotionController::isMovingState(MotionState s) const {
    return (s == MotionState::HomingLeft || s == MotionState::CalibMoveRight ||
            s == MotionState::MoveLeft  || s == MotionState::MoveRight);
}

void MotionController::updateHallHealth(uint32_t nowMs) {
    // Track rising edges (inactive->active) for each hall; counts as "end hit".
    if (st.hallL && !safety.lastHallL) safety.lastEndHitMs = nowMs;
    if (st.hallR && !safety.lastHallR) safety.lastEndHitMs = nowMs;
    safety.lastHallL = st.hallL;
    safety.lastHallR = st.hallR;

    // If we have never seen an end hit since boot, initialize to now to avoid false stall.
    if (safety.lastEndHitMs == 0) safety.lastEndHitMs = nowMs;
}

bool MotionController::evalLedShouldBeOn() const {
    if (led.mode == LedMode::Manual) return led.manualOn;

    // Auto mode:
    // If no clock is provided, treat as always-on (safe default for V1).
    if (!led.clockValid) return true;

    const uint16_t now = led.clockMin;
    const uint16_t start = led.onStartMin % 1440;
    const uint16_t end   = led.onEndMin % 1440;

    if (start == end) return true; // "always on" window
    if (start < end) {
        return (now >= start) && (now < end);
    }
    // overnight window (e.g., 20:00 -> 08:00)
    return (now >= start) || (now < end);
}

void MotionController::applyLedAndMotorPolicy(bool ledShouldBeOn) {
    // reflect current LED policy in status for UI
    st.ledOn = ledShouldBeOn;
    st.ledMode = led.mode;
    st.ledManualOn = led.manualOn;
    st.ledOnStartMin = led.onStartMin;
    st.ledOnEndMin = led.onEndMin;
    st.ledClockValid = led.clockValid;
    st.ledClockMin = led.clockMin;

    const uint32_t nowMs = millis();

    // When LED turns ON (system becomes active), reset safety timers to avoid false positives.
    if (ledShouldBeOn && !led.lastAppliedOn) {
        safety.lastStepPulseMs = nowMs;
        safety.lastEndHitMs = nowMs;
    }

    // LED is the "truth"; motor follows.
    digitalWrite(PIN_GROW_LED, ledShouldBeOn ? HIGH : LOW);
    led.lastAppliedOn = ledShouldBeOn;

    if (!ledShouldBeOn) {
        drv.enable(false);
        st.targetSps = 0;
        st.currentSps = 0;
    }
}

uint32_t MotionController::deriveNoEndTimeoutMs() const {
    // Conservative: expected travel time at min speed + dwell + margin.
    uint32_t steps = st.travelSteps;
    if (steps == 0) steps = 20000; // unknown travel; choose conservative large default
    float t = (float)steps / maxf(cfg.minSps, 1.0f);
    uint32_t travelMs = (uint32_t)(t * 1000.0f) + 5000;
    return travelMs + cfg.dwellMs + 2000;
}

void MotionController::rampSpeed(uint32_t nowMs, bool useDecel) {
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