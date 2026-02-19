#include "EnvelopeCodec.h"
#include <string.h>

namespace platform::envelope {

bool BedLinkBinaryCodec::decode(const uint8_t* p, uint16_t len, Envelope& out) {
    if (!p || len < 4) return false;
    out.capId = p[0];
    out.kind = static_cast<Kind>(p[1]);
    out.msgId = p[2];
    out.flags = p[3];

    uint16_t idx = 4;
    out.hasSeq = (out.flags & FLAG_HAS_SEQ) != 0;
    if (out.hasSeq) {
        if (len < 6) return false;
        out.seq = (uint16_t)p[4] | ((uint16_t)p[5] << 8);
        idx = 6;
    } else {
        out.seq = 0;
    }

    out.data = (idx <= len) ? (p + idx) : nullptr;
    out.dataLen = (idx <= len) ? (len - idx) : 0;
    return true;
}

uint16_t BedLinkBinaryCodec::encode(const Envelope& env, uint8_t* out, uint16_t outMax) {
    if (!out || outMax < 4) return 0;
    uint16_t idx = 0;
    out[idx++] = env.capId;
    out[idx++] = (uint8_t)env.kind;
    out[idx++] = env.msgId;

    uint8_t flags = env.flags;
    if (env.hasSeq) flags |= FLAG_HAS_SEQ;
    out[idx++] = flags;

    if (env.hasSeq) {
        if (outMax < idx + 2) return 0;
        out[idx++] = (uint8_t)(env.seq & 0xFF);
        out[idx++] = (uint8_t)((env.seq >> 8) & 0xFF);
    }

    if (env.dataLen > 0) {
        if (!env.data) return 0;
        if (outMax < idx + env.dataLen) return 0;
        memcpy(out + idx, env.data, env.dataLen);
        idx += env.dataLen;
    }

    return idx;
}

} // namespace platform::envelope
