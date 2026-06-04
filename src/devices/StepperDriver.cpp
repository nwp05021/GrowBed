#include "devices/StepperDriver.h"

namespace growbed::devices
{

bool StepperDriver::init()
{
    if (m_cfg.pinStep < 0 || m_cfg.pinDir < 0) {
        Serial.println("[Stepper] pinStep / pinDir ?¤ì • ?¤ë¥˜");
        return false;
    }

    pinMode(m_cfg.pinStep, OUTPUT);
    digitalWrite(m_cfg.pinStep, LOW);

    pinMode(m_cfg.pinDir, OUTPUT);
    setDirection(true);

    if (m_cfg.pinEnable >= 0) {
        pinMode(m_cfg.pinEnable, OUTPUT);
        disable();
    } else {
        m_enabled = true;
    }

    Serial.println("[Stepper] ì´ˆê¸°???„ë£Œ");
    return true;
}

void StepperDriver::enable()
{
    if (m_cfg.pinEnable >= 0)
        digitalWrite(m_cfg.pinEnable, m_cfg.invertEnable ? LOW : HIGH);
    m_enabled = true;
}

void StepperDriver::disable()
{
    if (m_cfg.pinEnable >= 0)
        digitalWrite(m_cfg.pinEnable, m_cfg.invertEnable ? HIGH : LOW);
    m_enabled = false;
}

void StepperDriver::setSpeed(uint32_t hz)
{
    bool wasZero  = (m_speedHz == 0U);
    m_speedHz     = hz;
    m_intervalUs  = (hz > 0U) ? (1'000'000U / hz) : 0U;
    // ?•ì? ???¬ì‹œ?????€?´ë¨¸ ì´ˆê¸°??(?¤ëž˜??nextStepUs ë¡??¸í•œ ì¦‰ë°œ ë°©ì?)
    if (hz > 0U && wasZero)
        m_nextStepUs = micros();
}

void StepperDriver::setDirection(bool forward)
{
    m_dirForward = forward;
    digitalWrite(m_cfg.pinDir, (forward ^ m_cfg.invertDir) ? HIGH : LOW);
}

void StepperDriver::doStep()
{
    digitalWrite(m_cfg.pinStep, HIGH);
    delayMicroseconds(m_cfg.stepPulseUs);
    digitalWrite(m_cfg.pinStep, LOW);
    m_position += m_dirForward ? 1 : -1;
}

void StepperDriver::tick()
{
    if (m_speedHz == 0U || !m_enabled) return;

    uint32_t now = micros();
    uint8_t emitted = 0;
    while (static_cast<int32_t>(now - m_nextStepUs) >= 0 && emitted < 8U) {
        doStep();
        m_nextStepUs += m_intervalUs;
        ++emitted;
        now = micros();
    }

    if (emitted == 8U && static_cast<int32_t>(now - m_nextStepUs) >= 0) {
        m_nextStepUs = now + m_intervalUs;
    }
}


} // namespace growbed::devices
