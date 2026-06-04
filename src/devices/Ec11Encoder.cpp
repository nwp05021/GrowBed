#include "devices/Ec11Encoder.h"
#include <Arduino.h>

namespace growbed::devices
{

namespace
{
Ec11Encoder* g_encoder = nullptr;

const int8_t kTable[4][4] = {
    { 0, -1,  1,  0},
    { 1,  0,  0, -1},
    {-1,  0,  0,  1},
    { 0,  1, -1,  0}
};
}

bool Ec11Encoder::init()
{
    pinMode(m_pinA,   INPUT_PULLUP);
    pinMode(m_pinB,   INPUT_PULLUP);
    pinMode(m_pinBtn, INPUT_PULLUP);

    int a = digitalRead(m_pinA);
    int b = digitalRead(m_pinB);
    m_lastState   = (a << 1) | b;
    m_detentState = m_lastState;
    m_stepAccum   = 0;

    g_encoder = this;
    attachInterrupt(digitalPinToInterrupt(m_pinA), Ec11Encoder::handleInterrupt, CHANGE);
    attachInterrupt(digitalPinToInterrupt(m_pinB), Ec11Encoder::handleInterrupt, CHANGE);
    return true;
}

int Ec11Encoder::consumeDelta()
{
    noInterrupts();
    int d = m_delta;
    m_delta = 0;
    interrupts();
    return d;
}

void Ec11Encoder::tick(uint32_t nowMs)
{
    bool raw = (digitalRead(m_pinBtn) == LOW);
    if (raw != m_btnRaw) {
        if (nowMs - m_lastBounceMs >= kDebounceMs) {
            m_lastBounceMs = nowMs;
            m_btnRaw = raw;
            if (raw) {
                m_btnDownMs   = nowMs;
                m_waitRelease = true;
            } else if (m_waitRelease) {
                uint32_t pressedMs = nowMs - m_btnDownMs;
                if (pressedMs >= kLongPressMs) {
                    m_btnLongPress = true;
                } else {
                    m_btnPressed = true;
                }
                m_waitRelease = false;
            }
        }
    }
}

bool Ec11Encoder::wasPressed()
{
    if (!m_btnPressed) return false;
    m_btnPressed = false;
    return true;
}

bool Ec11Encoder::wasLongPressed()
{
    if (!m_btnLongPress) return false;
    m_btnLongPress = false;
    return true;
}

void Ec11Encoder::handleInterrupt()
{
    if (g_encoder) g_encoder->pollEncoder();
}

void Ec11Encoder::pollEncoder()
{
    int a     = digitalRead(m_pinA);
    int b     = digitalRead(m_pinB);
    int state = (a << 1) | b;
    if (state == m_lastState) return;

    int    prev     = m_lastState;
    int8_t movement = kTable[prev][state];
    m_lastState     = state;

    if (movement == 0) return;

    m_stepAccum += movement;
    if (m_stepAccum > 8 || m_stepAccum < -8) {
        m_stepAccum = 0;
        return;
    }

    if (state == m_detentState) {
        uint32_t nowUs = micros();
        if (nowUs - m_lastEmitUs < kEncoderEmitGapUs) {
            m_stepAccum = 0;
            return;
        }

        if (m_stepAccum >= 3) {
            ++m_delta;
            m_lastEmitUs = nowUs;
        } else if (m_stepAccum <= -3) {
            --m_delta;
            m_lastEmitUs = nowUs;
        }
        m_stepAccum = 0;
    }
}

} // namespace growbed::devices
