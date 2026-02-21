
#include <Arduino.h>
#include "config/Defaults.h"
#include "app/controllers/MotionController.h"
#include "app/controllers/EncoderController.h"
#include "app/ui/UiController.h"
#include "product/growbed/GrowBedNode.h"
#include "platform/envelope/EnvelopeCodec.h"

#include "app/system/SettingsStore.h"

MotionConfig motionCfg;
UiConfig uiCfg;
EncoderConfig encCfg;

MotionController motion;
EncoderController enc;
UiController ui;

static SettingsStore store;
static PersistedData persist;

// ---- delayed persistence for config (debounced flash writes) ----
static bool gCfgDirty = false;
static uint32_t gCfgDirtySinceMs = 0;

// Called from UI when user commits parameter edits.
void markPersistDirty() {
    gCfgDirty = true;
    gCfgDirtySinceMs = millis();
}

product::growbed::GrowBedNode node;

void setup() {
    Serial.begin(115200);

    store.begin();

    bool ok = store.load(persist);
    if (!ok) {
        persist = PersistedData{};
        // 기본값은 MotionConfig 자체 default가 있음
        persist.cfg = motion.config();  
    }

    // 부팅 카운트 증가 후 즉시 저장
    persist.resetCount++;
    store.save(persist);

    // ✅ begin에 persist.cfg를 바로 넣는다 (핵심)
    motion.begin(persist.cfg);
        

    // apply persisted LED policy
    if (persist.ledMode == 0) motion.setLedModeAuto();
    else motion.setLedModeManual(persist.ledManualOn != 0);
    motion.setLedScheduleMinutes(persist.ledOnStartMin, persist.ledOnEndMin);

    // restore recent alerts
    motion.applyPersistedAlerts(persist.alertSeq, persist.alertHead, persist.alertCount,
                                persist.alertCodes, persist.alertUptimeSec);

    // restore last factory validation result
    motion.applyPersistedFactory(persist.factorySeq,
                                persist.factoryLastPass != 0,
                                persist.factoryFailCode,
                                persist.factoryFailStep,
                                persist.factoryLastDurationMs,
                                persist.factoryLastUptimeSec,
                                persist.factoryPassCount,
                                persist.factoryFailCount,
                                persist.factoryLogHead,
                                persist.factoryLogCount,
                                persist.factoryLogPass,
                                persist.factoryLogFailCode,
                                persist.factoryLogFailStep,
                                persist.factoryLogDurationSec,
                                persist.factoryLogUptimeSec,
                                persist.factoryLogCycles);

    node.begin(&motion);

    // Alert EVT -> LineBed transport (placeholder: Serial hex dump)
    motion.setAlertCallback([](uint8_t code, uint32_t /*seq*/, uint32_t uptimeMs, uint32_t cycles) {
        uint8_t data[16];
        platform::envelope::Envelope env;
        if (!node.buildEventAlert(env, data, sizeof(data), code, uptimeMs, cycles)) return;
        uint8_t payload[32];
        uint16_t n = platform::envelope::BedLinkBinaryCodec::encode(env, payload, sizeof(payload));
        if (n == 0) return;

        Serial.print("[EVT ALERT] ");
        for (uint16_t i = 0; i < n; i++) {
            if (payload[i] < 16) Serial.print('0');
            Serial.print(payload[i], HEX);
            Serial.print(' ');
        }
        Serial.println();

        // TODO: replace with RS485/BedLink transport to LineBed
        // e.g., Serial1.write(payload, n);
    });

    // Factory validation EVT -> LineBed transport (placeholder: Serial hex dump)
    motion.setFactoryCallback([](uint32_t seq, bool pass, uint8_t failCode, uint8_t failStep, uint32_t durationMs, uint32_t uptimeMs, uint32_t cycles) {
        uint8_t data[32];
        platform::envelope::Envelope env;
        if (!node.buildEventFactoryValidation(env, data, sizeof(data), seq, pass, failCode, failStep, durationMs, uptimeMs, cycles)) return;
        uint8_t payload[48];
        uint16_t n = platform::envelope::BedLinkBinaryCodec::encode(env, payload, sizeof(payload));
        if (n == 0) return;

        Serial.print("[EVT FACTORY] ");
        for (uint16_t i = 0; i < n; i++) {
            if (payload[i] < 16) Serial.print('0');
            Serial.print(payload[i], HEX);
            Serial.print(' ');
        }
        Serial.println();

        // TODO: replace with RS485/BedLink transport to LineBed
    });

    //-------------------------------------------
    // Auto Test
    //-------------------------------------------
    // motion.setLedModeManual(true);      // LED/Motor enable 강제
    // motion.setAutoHallIntervalMs(5000); // 5초 간격
    // motion.setAutoHallTest(true);
    // motion.requestStart();              // Stopped 상태면 홈부터 시작    

    motion.setMotionStallPulseTimeoutMs(2000);
    motion.setMotionStallNoEndTimeoutMs(0);

    motion.startFactoryAutoTest(2000);   // 2초 간격, 10 cycles 기본
    
    enc.begin(encCfg);
    ui.begin(uiCfg, &motion);
}

void loop() {
    EncoderEvents e = enc.poll();
    ui.handleEncoder(e);

    motion.tick();

    ui.tick();

    // Persist factory result whenever it changes (rare; OK to write immediately)
    {
        const auto& stF = motion.status();
        if (stF.factorySeq != persist.factorySeq) {
            persist.factorySeq = stF.factorySeq;
            persist.factoryLastPass = stF.factoryLastPass ? 1 : 0;
            persist.factoryFailCode = stF.factoryFailCode;
            persist.factoryFailStep = stF.factoryFailStep;
            persist.factoryLastDurationMs = stF.factoryLastDurationMs;
            persist.factoryLastUptimeSec = stF.factoryLastUptimeSec;
            persist.factoryPassCount = stF.factoryPassCount;
            persist.factoryFailCount = stF.factoryFailCount;
            store.save(persist);
        }
    }

    // Persist alert log whenever a new alert arrives (faults are rare; OK to write immediately)
    {
        const auto& stA = motion.status();
        if (stA.alertSeq != persist.alertSeq) {
            persist.alertSeq = stA.alertSeq;
            persist.alertHead = stA.alertHead;
            persist.alertCount = stA.alertCount;
            for (uint8_t i = 0; i < 5; i++) {
                persist.alertCodes[i] = stA.alertCodes[i];
                persist.alertUptimeSec[i] = stA.alertUptimeSec[i];
            }
            store.save(persist);
        }
    }

    static uint32_t lastLogMs = 0;
    uint32_t now = millis();

    // ---- debounced persistence for config/LED (flash/EEPROM wear reduction) ----
    if (gCfgDirty && (now - gCfgDirtySinceMs) >= 1000) {
        gCfgDirty = false;

        // motion config
        persist.cfg = motion.config();

        // LED policy snapshot from runtime status
        const auto& st2 = motion.status();
        persist.ledMode = (uint8_t)st2.ledMode;
        persist.ledManualOn = st2.ledManualOn ? 1 : 0;
        persist.ledOnStartMin = st2.ledOnStartMin;
        persist.ledOnEndMin = st2.ledOnEndMin;

        // alert log snapshot
        persist.alertSeq = st2.alertSeq;
        persist.alertHead = st2.alertHead;
        persist.alertCount = st2.alertCount;
        for (uint8_t i = 0; i < 5; i++) {
            persist.alertCodes[i] = st2.alertCodes[i];
            persist.alertUptimeSec[i] = st2.alertUptimeSec[i];
        }

        store.save(persist);
    }
    if (now - lastLogMs >= 1000) {
        lastLogMs = now;
        const auto& st = motion.status();
        Serial.print("state="); Serial.print((int)st.state);
        Serial.print(" sps="); Serial.print((int)st.currentSps);
        Serial.print(" pos="); Serial.print(st.pos);
        Serial.print(" Lraw="); Serial.print((int)st.hallRawL);
        Serial.print(" Lact="); Serial.print(st.hallL ? 1 : 0);
        Serial.print(" Rraw="); Serial.print((int)st.hallRawR);
        Serial.print(" Ract="); Serial.print(st.hallR ? 1 : 0);
        Serial.print(" err="); Serial.print((int)st.err);
        Serial.print(" travel="); Serial.print(st.travelSteps);
        Serial.print(" cyc="); Serial.println(st.cycles);
    }

    static uint8_t lastPerm = 0;
    const auto& st = motion.status();
    persist.faultTotal = st.faultTotal;
    persist.lastFaultCode = (uint8_t)st.lastErr;
    persist.lastFaultUptimeMs = st.lastFaultUptimeMs;

    if (st.permanentFault && !lastPerm) {
        store.save(persist);
    }
    lastPerm = st.permanentFault ? 1 : 0;
}
