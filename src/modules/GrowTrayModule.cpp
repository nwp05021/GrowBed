#include "modules/GrowTrayModule.h"

#include <Arduino.h>

namespace growbed::modules
{

bool GrowTrayModule::init()
{
    const bool leftOk = m_leftTouch.init();
    const bool rightOk = m_rightTouch.init();
    const bool stepperOk = m_stepper.init();

    m_stepper.setSpeed(0U);
    m_stepper.disable();
    m_on = false;

    Serial.printf("[GrowTray] init left=%u right=%u stepper=%u\n",
                  leftOk ? 1U : 0U,
                  rightOk ? 1U : 0U,
                  stepperOk ? 1U : 0U);
    return leftOk && rightOk && stepperOk;
}

void GrowTrayModule::setOn(bool on)
{
    if (m_on == on) return;
    m_on = on;

    if (m_on) {
        applyDetectedDirection();
        m_stepper.enable();
        m_currentStepHz = m_stepHz;
        m_stepper.setSpeed(m_stepHz);
    } else {
        m_directionState = DirectionState::Steady;
        m_currentStepHz = 0U;
        m_stepper.setSpeed(0U);
        m_stepper.disable();
    }
}

void GrowTrayModule::setStepHz(uint32_t hz)
{
    if (hz < kMinStepHz) hz = kMinStepHz;
    if (hz > kMaxStepHz) hz = kMaxStepHz;

    m_stepHz = hz;
    if (m_on && m_directionState == DirectionState::Steady) {
        m_currentStepHz = m_stepHz;
        m_stepper.setSpeed(m_currentStepHz);
    }
}

void GrowTrayModule::setCurrentDir(bool dir)
{
    requestDirection(dir);
}

void GrowTrayModule::tick(uint32_t nowMs)
{
    m_leftTouch.tick(nowMs);
    m_rightTouch.tick(nowMs);

    const bool leftTouched = m_leftTouch.rose();
    const bool rightTouched = m_rightTouch.rose();
    m_leftTouch.fell();
    m_rightTouch.fell();

    if (m_on) {
        if (leftTouched) {
            requestDirection(false);
        } else if (rightTouched) {
            requestDirection(true);
        }
        updateDirectionChange(nowMs);
    }

}

void GrowTrayModule::applyDetectedDirection()
{
    if (m_leftTouch.isDetected()) {
        requestDirection(false);
    } else if (m_rightTouch.isDetected()) {
        requestDirection(true);
    }
}

void GrowTrayModule::requestDirection(bool dir)
{
    if (dir == m_stepper.dirForward()) return;
    if (m_directionState != DirectionState::Steady && dir == m_requestedDir) return;

    m_requestedDir = dir;

    if (!m_on || m_currentStepHz == 0U) {
        m_stepper.setDirection(dir);
        m_directionState = DirectionState::Steady;
        return;
    }

    m_directionState = DirectionState::Decelerating;
    m_lastRampMs = millis();
}

void GrowTrayModule::updateDirectionChange(uint32_t nowMs)
{
    if (m_directionState == DirectionState::Steady) return;
    if (nowMs - m_lastRampMs < kRampIntervalMs) return;
    m_lastRampMs = nowMs;

    if (m_directionState == DirectionState::Decelerating) {
        if (m_currentStepHz <= kRampStepHz) {
            m_currentStepHz = 0U;
            m_stepper.setSpeed(0U);
            m_stepper.setDirection(m_requestedDir);
            m_currentStepHz = m_stepHz < kRampStepHz ? m_stepHz : kRampStepHz;
            m_stepper.setSpeed(m_currentStepHz);
            m_directionState = m_currentStepHz >= m_stepHz
                ? DirectionState::Steady
                : DirectionState::Accelerating;
        } else {
            m_currentStepHz -= kRampStepHz;
            m_stepper.setSpeed(m_currentStepHz);
        }
        return;
    }

    if (m_currentStepHz + kRampStepHz >= m_stepHz) {
        m_currentStepHz = m_stepHz;
        m_directionState = DirectionState::Steady;
    } else {
        m_currentStepHz += kRampStepHz;
    }
    m_stepper.setSpeed(m_currentStepHz);
}

} // namespace growbed::modules
