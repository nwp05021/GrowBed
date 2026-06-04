#pragma once
#include <cstdint>

namespace growbed::devices
{
    class Ec11Encoder
    {
    public:
        Ec11Encoder(int pinA, int pinB, int pinBtn)
            : m_pinA(pinA), m_pinB(pinB), m_pinBtn(pinBtn) {}

        bool init();
        int  consumeDelta();
        void tick(uint32_t nowMs);
        bool wasPressed();
        bool wasLongPressed();
        bool isButtonDown() const { return m_btnRaw; }
        uint32_t pressDurationMs(uint32_t nowMs) const { return m_btnRaw ? (nowMs - m_btnDownMs) : 0U; }

    private:
        int m_pinA, m_pinB, m_pinBtn;

        volatile int m_delta = 0;
        volatile int m_lastState = 0;
        int m_detentState = 0;
        volatile int m_stepAccum = 0;
        volatile uint32_t m_lastEmitUs = 0;

        bool     m_btnRaw = false;
        bool     m_btnPressed = false;
        bool     m_btnLongPress = false;
        bool     m_waitRelease = false;
        uint32_t m_btnDownMs = 0;
        uint32_t m_lastBounceMs = 0;

        static constexpr uint32_t kDebounceMs = 30U;
        static constexpr uint32_t kEncoderEmitGapUs = 1500U;
        static constexpr uint32_t kLongPressMs = 1500U;

        static void handleInterrupt();
        void pollEncoder();
    };
}
