#include "modules/GrowTrayModule.h"

#include <Arduino.h>

namespace growbed::modules
{

bool GrowTrayModule::init()
{
    const bool leftOk = m_leftTouch.init();
    const bool rightOk = m_rightTouch.init();
    const bool stepperOk = m_stepper.init();

    stopMotion();

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
    if (!m_on) {
        stopMotion();
        return;
    }

    updateLastDirFromTouches();
    startMotion(millis());
}

void GrowTrayModule::setStepHz(uint32_t hz)
{
    if (hz < kMinStepHz) hz = kMinStepHz;
    if (hz > kMaxStepHz) hz = kMaxStepHz;

    m_targetSpeedHz = hz;
    if (m_currentSpeedHz > m_targetSpeedHz) {
        m_currentSpeedHz = m_targetSpeedHz;
        if (m_state != State::Idle && m_state != State::StopPause) {
            m_stepper.setSpeed(m_currentSpeedHz);
        }
    }
}

void GrowTrayModule::setCurrentDir(bool dir)
{
    m_currDir = dir;
    m_motionStartMs = millis();
    m_endTouchArmed = false;
    m_stepper.setDirection(m_currDir);
}

void GrowTrayModule::tick(uint32_t nowMs)
{
    m_leftTouch.tick(nowMs);
    m_rightTouch.tick(nowMs);

    if (!m_on) return;

    if (m_state == State::Idle) {
        updateLastDirFromTouches();
        startMotion(nowMs);
        return;
    }

    if (m_state == State::StopPause) {
        if (nowMs - m_stopPauseStartMs >= kStopPauseMs) {
            updateLastDirFromTouches();
            startMotion(nowMs);
        }
        return;
    }

    if (endTouchDetected(nowMs) && m_state != State::Decel) {
        updateLastDirFromTouches();
        m_state = State::Decel;
    }

    m_stepper.tick();
    if (!stepOccurred()) return;

    if (m_state == State::Accel) {
        updateAccel();
    } else if (m_state == State::Decel) {
        updateDecel(nowMs);
    }
}

void GrowTrayModule::updateLastDirFromTouches()
{
    const bool left = m_leftTouch.isDetected();
    const bool right = m_rightTouch.isDetected();

    if (left) {
        m_currDir = false;
    } else if (right) {
        m_currDir = true;
    }
}

void GrowTrayModule::startMotion(uint32_t nowMs)
{
    m_currentSpeedHz = kMinStepHz;
    m_state = State::Accel;
    m_lastPosition = m_stepper.position();
    m_motionStartMs = nowMs;
    m_endTouchArmed = false;

    m_stepper.enable();
    m_stepper.setDirection(m_currDir);
    m_stepper.setSpeed(m_currentSpeedHz);

    Serial.printf("[GrowTray] move dir=%u\n", m_currDir ? 1U : 0U);
}

void GrowTrayModule::stopMotion()
{
    m_stepper.setSpeed(0U);
    m_stepper.disable();
    m_currentSpeedHz = 0U;
    m_state = State::Idle;
    m_endTouchArmed = false;
    m_lastPosition = m_stepper.position();
}

void GrowTrayModule::beginStopPause(uint32_t nowMs)
{
    m_stepper.setSpeed(0U);
    m_currentSpeedHz = 0U;
    m_stopPauseStartMs = nowMs;
    m_state = State::StopPause;
    Serial.printf("[GrowTray] end stop, pause=%lums\n",
                  static_cast<unsigned long>(kStopPauseMs));
}

bool GrowTrayModule::endTouchDetected(uint32_t nowMs)
{
    const bool targetTouch = m_currDir ? m_leftTouch.isDetected() : m_rightTouch.isDetected();

    if (nowMs - m_motionStartMs < kEndTouchArmDelayMs) {
        return false;
    }

    if (!targetTouch) {
        m_endTouchArmed = true;
        return false;
    }

    return m_endTouchArmed;
}

bool GrowTrayModule::stepOccurred()
{
    const int32_t position = m_stepper.position();
    if (position == m_lastPosition) return false;

    m_lastPosition = position;
    return true;
}

void GrowTrayModule::updateAccel()
{
    if (m_currentSpeedHz + kAccelHzPerStep >= m_targetSpeedHz) {
        m_currentSpeedHz = m_targetSpeedHz;
        m_state = State::Cruise;
    } else {
        m_currentSpeedHz += kAccelHzPerStep;
    }
    m_stepper.setSpeed(m_currentSpeedHz);
}

void GrowTrayModule::updateDecel(uint32_t nowMs)
{
    if (m_currentSpeedHz <= kMinStepHz + kAccelHzPerStep) {
        beginStopPause(nowMs);
        return;
    }

    m_currentSpeedHz -= kAccelHzPerStep;
    m_stepper.setSpeed(m_currentSpeedHz);
}

} // namespace growbed::modules
