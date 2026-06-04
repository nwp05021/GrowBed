#pragma once

#include "devices/HallSensorDriver.h"
#include "devices/StepperDriver.h"
#include <cstdint>

namespace growbed::modules
{
    class GrowTrayModule
    {
    public:
        GrowTrayModule(devices::HallSensorDriver& leftTouch,
                       devices::HallSensorDriver& rightTouch,
                       devices::StepperDriver& stepper)
            : m_leftTouch(leftTouch),
              m_rightTouch(rightTouch),
              m_stepper(stepper) {}

        bool init();
        void tick(uint32_t nowMs);

        void setOn(bool on);
        void on() { setOn(true); }
        void off() { setOn(false); }
        void setStepHz(uint32_t hz);
        void setCurrentDir(bool dir);

        bool isOn() const { return m_on; }
        bool isRunning() const { return m_state != State::Idle; }
        uint32_t stepHz() const { return m_targetSpeedHz; }
        bool currentDir() const { return m_currDir; }
        bool leftTouchDetected() const { return m_leftTouch.isDetected(); }
        bool rightTouchDetected() const { return m_rightTouch.isDetected(); }

    private:
        enum class State : uint8_t
        {
            Idle,
            Accel,
            Cruise,
            Decel,
            StopPause,
        };

        static constexpr uint32_t kMinStepHz = 80U;
        static constexpr uint32_t kDefaultStepHz = 300U;
        static constexpr uint32_t kMaxStepHz = 600U;
        static constexpr uint32_t kAccelHzPerStep = 40U;
        static constexpr uint32_t kStopPauseMs = 100U;
        static constexpr uint32_t kEndTouchArmDelayMs = 300U;

        devices::HallSensorDriver& m_leftTouch;
        devices::HallSensorDriver& m_rightTouch;
        devices::StepperDriver&    m_stepper;

        bool     m_on = false;
        bool     m_currDir = false;
        State    m_state = State::Idle;
        uint32_t m_currentSpeedHz = 0U;
        uint32_t m_targetSpeedHz = kDefaultStepHz;
        uint32_t m_stopPauseStartMs = 0U;
        uint32_t m_motionStartMs = 0U;
        bool     m_endTouchArmed = false;
        int32_t  m_lastPosition = 0;

        void updateLastDirFromTouches();
        void startMotion(uint32_t nowMs);
        void stopMotion();
        void beginStopPause(uint32_t nowMs);
        bool endTouchDetected(uint32_t nowMs);
        bool stepOccurred();
        void updateAccel();
        void updateDecel(uint32_t nowMs);
    };
}
