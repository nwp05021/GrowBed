
#include <Arduino.h>
#include "config/Defaults.h"
#include "app/controllers/MotionController.h"
#include "app/controllers/EncoderController.h"
#include "app/ui/UiController.h"
#include "product/growbed/GrowBedNode.h"

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
        
    node.begin(&motion);
    motion.begin(motionCfg);
    enc.begin(encCfg);
    ui.begin(uiCfg, &motion);
}

void loop() {
    EncoderEvents e = enc.poll();
    ui.handleEncoder(e);

    motion.tick();

    ui.tick();

    static uint32_t lastLogMs = 0;
    uint32_t now = millis();
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
