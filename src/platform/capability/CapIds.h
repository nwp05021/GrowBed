#pragma once
#include <stdint.h>

namespace platform::capability {

// V1.1 fixed mapping (SSOT: 23_capability_registry.md)
static constexpr uint8_t CAP_TELEMETRY_BASIC     = 0x01;
static constexpr uint8_t CAP_DIAGNOSTICS_HEALTH  = 0x02;
static constexpr uint8_t CAP_MOTION_LINEAR       = 0x10;
static constexpr uint8_t CAP_MOTION_CALIB        = 0x11;
static constexpr uint8_t CAP_GATEWAY_BEDHUB      = 0x80;

} // namespace platform::capability
