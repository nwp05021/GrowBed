#pragma once
#include "Envelope.h"

namespace platform::envelope {

// BedLink payload (binary envelope) codec
class BedLinkBinaryCodec {
public:
    static bool decode(const uint8_t* payload, uint16_t len, Envelope& out);
    static uint16_t encode(const Envelope& env, uint8_t* out, uint16_t outMax);
};

} // namespace platform::envelope
