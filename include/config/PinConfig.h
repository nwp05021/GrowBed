#pragma once
#include <cstdint>

namespace growbed::config
{
    struct Pin
    {
        // I2C0 (AHT20)
        static constexpr int I2C_SDA       = 4;
        static constexpr int I2C_SCL       = 5;

        // SPI0 (ST7789 TFT)
        static constexpr int TFT_SCLK      = 18;   // SPI0 SCK
        static constexpr int TFT_MOSI      = 19;   // SPI0 TX
        static constexpr int TFT_CS        = 17;
        static constexpr int TFT_DC        = 16;
        static constexpr int TFT_RST       = 20;

        // 
        static constexpr int SSR_HEATER     = 21;   // HIGH=ON
        static constexpr int SSR_HUMIDIFIER = 22;   // HIGH=ON
        static constexpr int BUZZER         = -1;   // 미사용
        // 
        static constexpr int FAN_PWM        = 7;
        static constexpr int FAN_PWM_CH     = 0;   // arduino-pico 미사용
        // EC11 로터리
        static constexpr int ENC_A          = 10;  // CLK
        static constexpr int ENC_B          = 11;  // DT
        static constexpr int ENC_BTN        = 12;  // SW Active LOW

        // RS485
        static constexpr int RS485_TX       = 0;
        static constexpr int RS485_RX       = 1;
        static constexpr int RS485_DE       = 13;
        static constexpr int RS485_RE       = -1;
        static constexpr uint32_t RS485_BAUD = 9600U;

        // A4988 / DRV8825
        static constexpr int STEP_STEP      = 2;  // STEP 펄스
        static constexpr int STEP_DIR       = 3;  // DIR 
        static constexpr int STEP_EN        = 6;  // ENABLE (LOW=활성, A4988/DRV8825 공통)
        static constexpr int STEP_SENSOR_L  = 8;  // 
        static constexpr int STEP_SENSOR_R  = 9;  // 
        // MS1/MS2/MS3 마이크로스탭

        // CDS GL5537 조도센서
        static constexpr int CDS_GL5537     = 26;  // ADC 

        // LED
        static constexpr int LED_PWM        = 23;  // PWM 밝기
    };
}
