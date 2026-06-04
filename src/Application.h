#pragma once

#include "config/PinConfig.h"
#include "config/AppConfig.h"

#include "devices/I2cBus.h"
#include "devices/Aht20Driver.h"
#include "devices/RtcDriver.h"
#include "devices/GpioOutput.h"
#include "devices/PwmFan.h"
#include "devices/St7789Display.h"
#include "devices/Ec11Encoder.h"
#include "devices/CdsGl5537Driver.h"
#include "devices/GrowLightDriver.h"
#include "devices/HallSensorDriver.h"
#include "devices/StepperDriver.h"
#include "devices/Rs485Driver.h"

#include "domain/AppSettings.h"
#include "domain/RuntimeState.h"
#include "domain/GrowSession.h"

#include "storage/NvsStorage.h"

#include "modules/SensorManager.h"
#include "modules/ClimateModule.h"
#include "modules/Rs485Module.h"
#include "modules/GrowLightModule.h"
#include "modules/GrowTrayModule.h"
#include "modules/GrowManager.h"

#include "app/AppController.h"
#include "infra/ProvisioningManager.h"

#include "ui/UiModel.h"
#include "ui/UiController.h"
#include "ui/MainUiRenderer.h"

namespace growbed
{

class Application
{
public:
    Application();

    void init();
    void tick();

private:
    devices::I2cBus            m_i2c;
    devices::Aht20Driver       m_aht20;
    devices::RtcDriver         m_rtc;
    devices::GpioOutput        m_heater;
    devices::GpioOutput        m_humidifier;
    devices::GpioOutput        m_buzzer;
    devices::PwmFan            m_fan;
    devices::CdsGl5537Driver   m_cdsLightSensor;
    devices::GrowLightDriver   m_growLightDriver;
    devices::HallSensorDriver  m_leftTouch;
    devices::HallSensorDriver  m_rightTouch;
    devices::St7789Display     m_display;
    devices::Ec11Encoder       m_encoder;
    devices::StepperDriver     m_stepper;
    devices::Rs485Driver       m_rs485;

    domain::AppSettings  m_settings;
    domain::RuntimeState m_state;
    domain::GrowSession  m_session;

    storage::NvsStorage  m_nvs;

    modules::SensorManager  m_sensorMgr;
    modules::ClimateModule  m_climate;
    modules::Rs485Module    m_rs485Module;
    modules::GrowLightModule m_growLightModule;
    modules::GrowTrayModule  m_growTrayModule;
    modules::GrowManager     m_growManager;

    app::AppController         m_appCtrl;
    infra::ProvisioningManager m_provisioning;

    ui::UiModel        m_uiModel;
    ui::UiController   m_uiCtrl;
    ui::MainUiRenderer m_renderer;

    void syncGrowState();
};

} // namespace growbed
