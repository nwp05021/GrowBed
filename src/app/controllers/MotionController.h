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
    BothLimitsActive = 4,
    MotionStall = 5
};

enum class LedMode : uint8_t {
    Auto = 0,
    Manual = 1
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

    // ---- LED policy status (for UI/diagnostics) ----
    bool ledOn = false;
    LedMode ledMode = LedMode::Auto;
    bool ledManualOn = false;
    uint16_t ledOnStartMin = 0; // minutes since midnight
    uint16_t ledOnEndMin = 0;   // minutes since midnight
    bool ledClockValid = false;
    uint16_t ledClockMin = 0;

    // ---- Alerts (Fault -> LineBed EVT + UI recent log) ----
    uint32_t alertSeq = 0;              // increments every alert
    uint8_t  alertHead = 0;             // ring buffer head (next write index)
    uint8_t  alertCount = 0;            // <= 5
    uint8_t  alertCodes[5] = {0};       // last alerts (ring)
    uint32_t alertUptimeSec[5] = {0};   // seconds since boot at alert time
    bool     alertPending = false;      // true when a new alert is queued (consumable)
    uint8_t  alertPendingCode = 0;

    // ---- Factory Validation result (persisted) ----
    uint32_t factorySeq = 0;
    bool     factoryLastPass = false;
    uint8_t  factoryFailCode = 0;
    uint8_t  factoryFailStep = 0;
    uint32_t factoryLastDurationMs = 0;
    uint32_t factoryLastUptimeSec = 0;
    uint32_t factoryPassCount = 0;
    uint32_t factoryFailCount = 0;

    // ---- Factory Validation history log (ring, max 8) ----
    uint8_t  factoryLogHead = 0;
    uint8_t  factoryLogCount = 0;
    uint8_t  factoryLogPass[8] = {0};
    uint8_t  factoryLogFailCode[8] = {0};
    uint8_t  factoryLogFailStep[8] = {0};
    uint16_t factoryLogDurationSec[8] = {0};
    uint32_t factoryLogUptimeSec[8] = {0};
    uint32_t factoryLogCycles[8] = {0};
};

class MotionController {
public:
    // Alert callback (e.g., send to LineBed). Called at fault time.
    using AlertCallback = void(*)(uint8_t code, uint32_t seq, uint32_t uptimeMs, uint32_t cycles);
    using FactoryCallback = void(*)(uint32_t seq, bool pass, uint8_t failCode, uint8_t failStep,
                                    uint32_t durationMs, uint32_t uptimeMs, uint32_t cycles);

public:
    // UI/Test can temporarily mute popups/alerts while running scripted validation.
    // Safety behavior (motor disable, state transitions) remains intact.
    void setUiMuteSeconds(uint16_t seconds);
    bool isUiMuteActive() const;

    void begin(const MotionConfig& cfg_);

    // Restore alert ring buffer from persisted storage.
    void applyPersistedAlerts(uint32_t seq, uint8_t head, uint8_t count,
                              const uint8_t* codes, const uint32_t* uptimeSec);

    // Restore persisted factory validation result + history log.
    void applyPersistedFactory(uint32_t seq, bool lastPass, uint8_t failCode, uint8_t failStep,
                               uint32_t durationMs, uint32_t uptimeSec,
                               uint32_t passCount, uint32_t failCount,
                               uint8_t logHead, uint8_t logCount,
                               const uint8_t* logPass, const uint8_t* logFailCode, const uint8_t* logFailStep,
                               const uint16_t* logDurationSec, const uint32_t* logUptimeSec, const uint32_t* logCycles);

    // Record a factory validation result (called by UI).
    void recordFactoryResult(bool pass, uint8_t failCode, uint8_t failStep, uint32_t durationMs, uint32_t uptimeMs);

    void setAlertCallback(AlertCallback cb);
    void setFactoryCallback(FactoryCallback cb);

    // UI can call this after showing a popup.
    void acknowledgeAlert(uint32_t seq);

    void applyConfig(const MotionConfig& cfg_);

    const MotionConfig& config() const;
    const MotionStatus& status() const;

    // ---- LED policy / Motor enable linkage ----
    void setLedModeAuto();
    void setLedModeManual(bool on);
    void setLedScheduleMinutes(uint16_t onStartMin, uint16_t onEndMin);
    void setClockMinutes(uint16_t minutesSinceMidnight);

    // Safety timeouts (can be tuned from UI/engineering later)
    void setMotionStallPulseTimeoutMs(uint32_t ms);
    void setMotionStallNoEndTimeoutMs(uint32_t ms);

    // ---- UI-facing request API (non-blocking) ----
    void requestStart();
    void requestStop();
    void requestHome();
    void requestRecalibrate();
    // Engineering
    void requestForceMoveLeft();
    void requestForceMoveRight();
    void requestDisableMotor();
    void requestInjectFault(MotionError e);
    void requestSetMaxSps(float sps);
    void requestSetAccel(float a);
    void requestSetDwell(uint32_t ms);
    void requestSetRehomeEvery(uint32_t cycles);

    // ---- Test hooks (UI/Test menu) ----
    void requestSimulateHallLeft(uint16_t activeMs);
    void requestSimulateHallRight(uint16_t activeMs);

    void tick();

    // ---- Factory Auto Validation (default 10 cycles) ----
    void startFactoryAutoTest(uint32_t hallIntervalMs = 5000, uint16_t targetCycles = 10);
    void stopFactoryAutoTest();
    bool isFactoryAutoTestRunning() const;

    uint16_t factoryAutoTargetCycles() const;
    uint16_t factoryAutoStartCycles() const;

private:
    // ---- internal sync helpers ----
    void syncAlertStatus();
    void syncFactoryStatus();
    void pushFactoryLog(bool pass, uint8_t failCode, uint8_t failStep,
                        uint32_t durationMs, uint32_t uptimeMs, uint32_t cycles);

    // ---- internal helpers ----
    static float maxf(float a, float b);

    void resetForHoming(bool userInitiated);
    void enterStopped(uint32_t nowMs);
    void fault(MotionError e);

    void enterDwell(uint32_t nowMs, MotionState next);
    void enterForcedMove(bool toRight);

    bool stepDue(uint32_t nowUs);
    void doStep(bool forward, uint32_t nowMs, uint32_t nowUs);
    bool isMovingState(MotionState s) const;

    void updateHallHealth(uint32_t nowMs);

    bool evalLedShouldBeOn() const;
    void applyLedAndMotorPolicy(bool ledShouldBeOn);

    uint32_t deriveNoEndTimeoutMs() const;

    void rampSpeed(uint32_t nowMs, bool useDecel);

    // ---- Auto Hall Toggle test (no real sensors needed) ----
    void setAutoHallTest(bool enabled);
    bool isAutoHallTestEnabled() const;

    void setAutoHallIntervalMs(uint32_t intervalMs);   // e.g. 1000~10000
    uint32_t autoHallIntervalMs() const;    

private:
    MotionConfig cfg;
    StepperHal_Drv8825 drv;
    MotionStatus st;

    struct {
        uint32_t seq = 0;
        uint8_t head = 0;
        uint8_t count = 0;
        uint8_t codes[5] = {0};
        uint32_t uptimeSec[5] = {0};
        bool pending = false;
        uint8_t pendingCode = 0;
        AlertCallback cb = nullptr;
    } alerts;

    struct {
        uint32_t seq = 0;
        bool lastPass = false;
        uint8_t failCode = 0;
        uint8_t failStep = 0;
        uint32_t durationMs = 0;
        uint32_t uptimeSec = 0;
        uint32_t passCount = 0;
        uint32_t failCount = 0;

        // history log (ring, max 8)
        uint8_t  logHead = 0;
        uint8_t  logCount = 0;
        uint8_t  logPass[8] = {0};
        uint8_t  logFailCode[8] = {0};
        uint8_t  logFailStep[8] = {0};
        uint16_t logDurationSec[8] = {0};
        uint32_t logUptimeSec[8] = {0};
        uint32_t logCycles[8] = {0};

        FactoryCallback cb = nullptr;
    } factory;

    struct LedPolicy {
        LedMode mode = LedMode::Auto;
        bool manualOn = true;
        uint16_t onStartMin = 8 * 60;   // 08:00
        uint16_t onEndMin   = 20 * 60;  // 20:00
        bool clockValid = false;
        uint16_t clockMin = 0;
        bool lastAppliedOn = false;
    } led;

    struct SafetyPolicy {
        // Pulse stall: if moving state but no step pulse for this long => MotionStall
        uint32_t pulseStallTimeoutMs = 800;
        // End-sensor liveness: if no hall hit for too long while active => MotionStall
        // 0 means auto-derive from travel + dwell
        uint32_t noEndTimeoutMs = 0;
        // Both sensors active debounce
        uint32_t bothActiveDebounceMs = 20;

        uint32_t lastStepPulseMs = 0;
        uint32_t lastEndHitMs = 0;
        bool lastHallL = false;
        bool lastHallR = false;
    } safety;

    uint32_t stateEnterMs = 0;
    uint32_t lastStepUs = 0;
    uint32_t lastRampMs = 0;

    uint32_t calibSteps = 0;
    uint32_t moveSteps = 0;
    bool lastWasRightEnd = false;

    uint32_t bothActiveSinceMs = 0;

    MotionState nextAfterDwell = MotionState::MoveRight;

    struct SimHall {
        bool leftActive = false;
        bool rightActive = false;
        uint32_t leftUntilMs = 0;
        uint32_t rightUntilMs = 0;
    } sim;

    // UI/Test mute window (ms). Used to suppress popups/alerts while scripted validation is running.
    uint32_t uiMuteUntilMs = 0;

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

    struct AutoHallTest {
        bool enabled = false;
        uint32_t intervalMs = 3000;     // default 3s
        uint32_t pulseMs = 120;         // how long to assert hall "true"
        uint32_t lastToggleMs = 0;
        bool nextLeft = true;
    } autoHall;  
    
    struct FactoryAutoTest {
        bool running = false;
        uint16_t targetCycles = 10;
        uint16_t startCycles = 0;
        uint32_t startMs = 0;
        uint8_t failStep = 1;
    } fauto;
};