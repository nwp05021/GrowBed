#include "Application.h"
#include <Arduino.h>
#include <LittleFS.h>

namespace growbed
{

Application::Application()
    : m_aht20(m_i2c),
      m_heater(config::Pin::SSR_HEATER),
      m_humidifier(config::Pin::SSR_HUMIDIFIER),
      m_buzzer(config::Pin::BUZZER),
      m_fan(config::Pin::FAN_PWM, 0),
      m_cdsLightSensor(devices::CdsGl5537Driver::Config{}),
      m_growLightDriver(devices::GrowLightDriver::Config{
          .pin = config::Pin::LED_PWM,
      }),
      m_leftTouch(devices::HallSensorDriver::Config{
          .pin = config::Pin::STEP_SENSOR_L,
          .activeLow = false,
      }),
      m_rightTouch(devices::HallSensorDriver::Config{
          .pin = config::Pin::STEP_SENSOR_R,
          .activeLow = false,
      }),
      m_encoder(config::Pin::ENC_A, config::Pin::ENC_B, config::Pin::ENC_BTN),
      m_rs485(Serial1),
      m_stepper(devices::StepperDriver::Config{
          .pinStep     = config::Pin::STEP_STEP,
          .pinDir      = config::Pin::STEP_DIR,
          .pinEnable   = config::Pin::STEP_EN,
      }),

      m_settings(domain::AppSettings::defaults()),
      m_state(domain::RuntimeState::zero()),

      m_sensorMgr(m_aht20, m_state),
      m_climate(m_state, m_settings, m_heater, m_humidifier, m_buzzer),
      m_rs485Module(m_state, m_rs485),
      m_growLightModule(m_cdsLightSensor, m_growLightDriver),
      m_growTrayModule(m_leftTouch, m_rightTouch, m_stepper),
      m_growManager(m_growLightModule, m_growTrayModule),

      m_appCtrl(m_state, m_settings, m_session,
                m_nvs, m_heater, m_humidifier, m_growLightModule, m_growTrayModule, m_fan),

      m_uiCtrl(m_uiModel, m_state, m_appCtrl, m_provisioning, m_encoder, m_rtc),
      m_renderer(m_uiModel, m_display)
{
}

void Application::init()
{
    Serial.begin(115200);
    Serial.printf("=== GrowBed FW %s boot ===\n", GROWBED_FW_VERSION);

    if (!LittleFS.begin()) {
        Serial.println("[App] LittleFS format & mount...");
        LittleFS.format();
        LittleFS.begin();
    }

    if (!m_nvs.init()) {
        Serial.println("[App] NvsStorage init failed");
    }

    m_appCtrl.restoreFromStorage();

    if (!m_i2c.init(config::Pin::I2C_SDA, config::Pin::I2C_SCL, 400000U)) {
        Serial.println("[App] I2C init failed");
    }

    if (m_rtc.init(m_nvs)) {
        if (m_rtc.isValid()) m_rtc.syncSystemTime();
        else Serial.println("[App] RTC time invalid ??set via menu");
    }

    m_aht20.init();

    m_heater.init();
    m_humidifier.init();
    m_buzzer.init();
    m_fan.init();
    m_rs485Module.init(devices::Rs485Driver::Config{
        .pinTx = config::Pin::RS485_TX,
        .pinRx = config::Pin::RS485_RX,
        .pinDe = config::Pin::RS485_DE,
        .pinRe = config::Pin::RS485_RE,
        .baud  = config::Pin::RS485_BAUD,
    });

    m_growLightModule.init();
    m_growTrayModule.init();
    m_growManager.init();

    if (!m_display.init()) {
        Serial.println("[App] Display init failed");
    }
    m_encoder.init();

    // 부???�면
    {
        auto& gfx = m_display.gfx();
        gfx.startWrite();
        gfx.fillScreen(0x0000);
        gfx.setTextColor(0x07E0, 0x0000); // ?�색
        gfx.setTextSize(2);
        gfx.setCursor(40, 90);
        gfx.print("GrowBed");
        gfx.setTextColor(0xFFFF, 0x0000);
        gfx.setTextSize(1);
        gfx.setCursor(80, 120);
        gfx.print(GROWBED_FW_VERSION);
        gfx.setCursor(50, 140);
        gfx.print("Initializing...");
        gfx.endWrite();
        delay(800);
    }

    std::snprintf(m_uiModel.fwVersion, sizeof(m_uiModel.fwVersion), GROWBED_FW_VERSION);
    Serial.println("[App] Setup complete");
}

void Application::tick()
{
    uint32_t now = millis();

    m_sensorMgr.tick(now);
    m_climate.tick(now);
    m_rs485Module.tick(now);
    if (!m_state.manualMode) {
        m_growManager.tick(now);
    }
    m_growLightModule.tick(now);
    m_growTrayModule.tick(now);
    syncGrowState();
    m_encoder.tick(now);
    m_uiCtrl.tick(now);
    m_renderer.render(now);

    m_state.uptimeMs = now;
}

void Application::syncGrowState()
{
    m_state.lightSensorOk    = m_cdsLightSensor.isConnected();
    m_state.ambientLux       = static_cast<uint16_t>(m_growLightModule.ambientLux());
    m_state.ledOn            = m_growLightDriver.isOn();
    m_state.ledBrightnessPct = m_growLightModule.brightnessPercent();
    m_state.trayOn           = m_growTrayModule.isOn();
    m_state.trayStepHz       = m_growTrayModule.stepHz();
    m_state.trayLeftTouch    = m_growTrayModule.leftTouchDetected();
    m_state.trayRightTouch   = m_growTrayModule.rightTouchDetected();
}

} // namespace growbed
