#pragma once
#include <stdint.h>

namespace platform::capability {

// motion.linear (CAP 0x10)
static constexpr uint8_t ML_START      = 0x01;
static constexpr uint8_t ML_STOP       = 0x02;
static constexpr uint8_t ML_HOME       = 0x03;
static constexpr uint8_t ML_SET_SPEED  = 0x04; // int16 sps
static constexpr uint8_t ML_SET_DWELL  = 0x05; // uint16 ms

} // namespace platform::capability
