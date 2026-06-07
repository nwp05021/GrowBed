#include "devices/StepperDriver.h"

#include <hardware/clocks.h>
#include <hardware/pwm.h>

namespace growbed::devices
{

bool StepperDriver::init()
{
    if (m_cfg.pinStep < 0 || m_cfg.pinDir < 0) {
        Serial.println("[Stepper] invalid STEP/DIR pin");
        return false;
    }

    m_pwmSlice = pwm_gpio_to_slice_num(m_cfg.pinStep);
    m_pwmChannel = pwm_gpio_to_channel(m_cfg.pinStep);

    gpio_set_function(m_cfg.pinStep, GPIO_FUNC_PWM);
    pwm_set_chan_level(m_pwmSlice, m_pwmChannel, 0U);
    pwm_set_enabled(m_pwmSlice, false);

    pinMode(m_cfg.pinDir, OUTPUT);
    setDirection(true);

    if (m_cfg.pinEnable >= 0) {
        pinMode(m_cfg.pinEnable, OUTPUT);
        disable();
    } else {
        m_enabled = true;
    }

    m_initialized = true;
    updatePulseOutput();
    Serial.printf("[Stepper] hardware PWM initialized (slice=%u channel=%u)\n",
                  m_pwmSlice, m_pwmChannel);
    return true;
}

void StepperDriver::enable()
{
    if (m_cfg.pinEnable >= 0) {
        digitalWrite(m_cfg.pinEnable, m_cfg.invertEnable ? LOW : HIGH);
    }
    m_enabled = true;
    updatePulseOutput();
}

void StepperDriver::disable()
{
    m_enabled = false;
    updatePulseOutput();

    if (m_cfg.pinEnable >= 0) {
        digitalWrite(m_cfg.pinEnable, m_cfg.invertEnable ? HIGH : LOW);
    }
}

void StepperDriver::setSpeed(uint32_t hz)
{
    m_speedHz = hz;
    updatePulseOutput();
}

void StepperDriver::setDirection(bool forward)
{
    m_dirForward = forward;
    digitalWrite(m_cfg.pinDir, (forward ^ m_cfg.invertDir) ? HIGH : LOW);
}

void StepperDriver::updatePulseOutput()
{
    if (!m_initialized || !m_enabled || m_speedHz == 0U) {
        pwm_set_enabled(m_pwmSlice, false);
        pwm_set_chan_level(m_pwmSlice, m_pwmChannel, 0U);
        return;
    }

    const uint32_t clockHz = clock_get_hz(clk_sys);
    float divider = static_cast<float>(clockHz)
                  / (static_cast<float>(m_speedHz) * 65'536.0f);
    if (divider < 1.0f) divider = 1.0f;
    if (divider > 255.0f) divider = 255.0f;

    uint32_t periodCounts = static_cast<uint32_t>(
        static_cast<float>(clockHz) / (divider * static_cast<float>(m_speedHz)) + 0.5f);
    if (periodCounts < 2U) periodCounts = 2U;
    if (periodCounts > 65'536U) periodCounts = 65'536U;

    const uint16_t wrap = static_cast<uint16_t>(periodCounts - 1U);
    pwm_set_enabled(m_pwmSlice, false);
    pwm_set_clkdiv(m_pwmSlice, divider);
    pwm_set_wrap(m_pwmSlice, wrap);
    pwm_set_chan_level(m_pwmSlice, m_pwmChannel, periodCounts / 2U);
    pwm_set_counter(m_pwmSlice, 0U);
    pwm_set_enabled(m_pwmSlice, true);
}

} // namespace growbed::devices
