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
        bool isRunning() const { return m_on; }
        uint32_t stepHz() const { return m_stepHz; }
        bool currentDir() const { return m_stepper.dirForward(); }
        bool leftTouchDetected() const { return m_leftTouch.isDetected(); }
        bool rightTouchDetected() const { return m_rightTouch.isDetected(); }

    private:
        enum class DirectionState : uint8_t
        {
            Steady,
            Decelerating,
            Accelerating,
        };

        static constexpr uint32_t kMinStepHz = 80U;
        static constexpr uint32_t kDefaultStepHz = 300U;
        static constexpr uint32_t kMaxStepHz = 600U;
        static constexpr uint32_t kRampStepHz = 20U;
        static constexpr uint32_t kRampIntervalMs = 20U;

        devices::HallSensorDriver& m_leftTouch;
        devices::HallSensorDriver& m_rightTouch;
        devices::StepperDriver& m_stepper;

        bool m_on = false;
        uint32_t m_stepHz = kDefaultStepHz;
        uint32_t m_currentStepHz = 0U;
        bool m_requestedDir = true;
        DirectionState m_directionState = DirectionState::Steady;
        uint32_t m_lastRampMs = 0U;

        void applyDetectedDirection();
        void requestDirection(bool dir);
        void updateDirectionChange(uint32_t nowMs);
    };
}
