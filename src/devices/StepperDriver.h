#pragma once
#include <Arduino.h>
#include <cstdint>

// A4988 / DRV8825 ?�태??모터 로우?�벨 ?�라?�버.
// ?�도·방향·?�성?��? ?�시간으�??�어?�니??
// 가감속 ?�로?�일?� ?�위 StepperModule ?�서 ?�당?�니??
// 마이?�로?�텝 ?�상??MS1/MS2/MS3)???� ?�퍼�??�드?�어 ?�정?�세??
namespace growbed::devices
{
    class StepperDriver
    {
    public:
        struct Config
        {
            int      pinStep      = -1;
            int      pinDir       = -1;
            int      pinEnable    = -1;   // 미사?�시 -1 (??�� ?�성 처리)
            bool     invertEnable = true; // A4988/DRV8825: ENABLE LOW=?�성
            bool     invertDir    = false;
            uint32_t stepPulseUs  = 2U;   // STEP ?�스 ??µs (A4988??, DRV8825??)
        };

        explicit StepperDriver(const Config& cfg) : m_cfg(cfg) {}

        // GPIO 초기화 pinStep / pinDir 필수
        bool init();

        void enable();
        void disable();
        bool isEnabled()  const { return m_enabled; }

        void setSpeed(uint32_t hz);

        void setDirection(bool forward);

        uint32_t speed()      const { return m_speedHz; }
        bool     dirForward() const { return m_dirForward; }

        void tick();

        int32_t position()  const { return m_position; }
        void resetPosition()      { m_position = 0; }

    private:
        Config   m_cfg;
        bool     m_enabled    = false;
        bool     m_dirForward = true;
        int32_t  m_position   = 0;

        uint32_t m_speedHz    = 0U;
        uint32_t m_intervalUs = 0U;
        uint32_t m_nextStepUs = 0U;

        void doStep();
    };
}
