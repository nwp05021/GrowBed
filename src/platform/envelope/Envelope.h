#pragma once
#include <Arduino.h>
#include <stdint.h>

namespace platform::envelope {

enum class Kind : uint8_t {
    Cmd = 0x01,
    Tel = 0x02,
    Ack = 0x03,
    Err = 0x04,
};

struct Envelope {
    uint8_t capId {0};
    Kind kind {Kind::Tel};
    uint8_t msgId {0};
    uint8_t flags {0};
    bool hasSeq {false};
    uint16_t seq {0};

    // V1: raw bytes to avoid tight coupling.
    const uint8_t* data {nullptr};
    uint16_t dataLen {0};
};

// FLAGS bits (V1.1)
static constexpr uint8_t FLAG_REQ_ACK = 0x01;
static constexpr uint8_t FLAG_HAS_SEQ = 0x02;

} // namespace platform::envelope
