
#include <Arduino.h>
#include "config/Defaults.h"
#include "app/controllers/MotionController.h"
#include "app/controllers/EncoderController.h"
#include "app/ui/UiController.h"
#include "product/growbed/GrowBedNode.h"

MotionConfig motionCfg;
UiConfig uiCfg;
EncoderConfig encCfg;

MotionController motion;
EncoderController enc;
UiController ui;

product::growbed::GrowBedNode node;

void setup() {
    Serial.begin(115200);
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
}
