#include "GrowBedNode.h"
#include "../../platform/capability/CapIds.h"
#include "../../platform/capability/MotionLinearMsgs.h"
#include "../../app/controllers/MotionController.h"

namespace product::growbed {

void GrowBedNode::begin(MotionController* motion) { _motion = motion; }

bool GrowBedNode::handleCommand(const platform::envelope::Envelope& cmd,
                               platform::envelope::Envelope& outReply,
                               uint8_t* replyDataBuf, uint16_t replyDataMax) {
    if (!_motion) return false;
    if (cmd.kind != platform::envelope::Kind::Cmd) return false;

    outReply.capId = cmd.capId;
    outReply.kind = platform::envelope::Kind::Ack;
    outReply.msgId = cmd.msgId;
    outReply.flags = 0;
    outReply.hasSeq = cmd.hasSeq;
    outReply.seq = cmd.seq;

    uint8_t status = 0;

    if (cmd.capId == platform::capability::CAP_MOTION_LINEAR) {
        switch (cmd.msgId) {
            case platform::capability::ML_START:
            case platform::capability::ML_STOP:
            case platform::capability::ML_HOME:
                status = 0; // scaffold: no-op
                break;
            case platform::capability::ML_SET_SPEED:
            case platform::capability::ML_SET_DWELL:
                status = 0; // TODO parse/apply
                break;
            default:
                outReply.kind = platform::envelope::Kind::Err;
                status = 2; // UnknownMsgId
                break;
        }
    } else {
        outReply.kind = platform::envelope::Kind::Err;
        status = 1; // UnknownCap
    }

    if (replyDataBuf && replyDataMax >= 1) {
        replyDataBuf[0] = status;
        outReply.data = replyDataBuf;
        outReply.dataLen = 1;
    } else {
        outReply.data = nullptr;
        outReply.dataLen = 0;
    }
    return true;
}

bool GrowBedNode::buildTelemetryBasic(platform::envelope::Envelope& outTel,
                                     uint8_t* dataBuf, uint16_t dataMax) {
    if (!_motion || !dataBuf || dataMax < 8) return false;

    const auto& st = _motion->status();
    dataBuf[0] = (uint8_t)st.state;
    dataBuf[1] = (uint8_t)st.err;
    uint32_t up = millis();
    dataBuf[2] = (uint8_t)(up & 0xFF);
    dataBuf[3] = (uint8_t)((up >> 8) & 0xFF);
    dataBuf[4] = (uint8_t)((up >> 16) & 0xFF);
    dataBuf[5] = (uint8_t)((up >> 24) & 0xFF);
    dataBuf[6] = 0;
    dataBuf[7] = 0;

    outTel.capId = platform::capability::CAP_TELEMETRY_BASIC;
    outTel.kind = platform::envelope::Kind::Tel;
    outTel.msgId = 0x01;
    outTel.flags = 0;
    outTel.hasSeq = false;
    outTel.seq = 0;
    outTel.data = dataBuf;
    outTel.dataLen = 8;
    return true;
}

bool GrowBedNode::buildEventAlert(platform::envelope::Envelope& outEvt,
                                 uint8_t* dataBuf, uint16_t dataMax,
                                 uint8_t faultCode, uint32_t uptimeMs, uint32_t cycles) {
    if (!dataBuf || dataMax < 13) return false;

    // DATA:
    // 0: faultCode
    // 1: state (snapshot)
    // 2..5: uptimeMs (u32)
    // 6..9: cycles (u32)
    // 10..12: reserved
    dataBuf[0] = faultCode;
    dataBuf[1] = _motion ? (uint8_t)_motion->status().state : 0;

    dataBuf[2] = (uint8_t)(uptimeMs & 0xFF);
    dataBuf[3] = (uint8_t)((uptimeMs >> 8) & 0xFF);
    dataBuf[4] = (uint8_t)((uptimeMs >> 16) & 0xFF);
    dataBuf[5] = (uint8_t)((uptimeMs >> 24) & 0xFF);

    dataBuf[6] = (uint8_t)(cycles & 0xFF);
    dataBuf[7] = (uint8_t)((cycles >> 8) & 0xFF);
    dataBuf[8] = (uint8_t)((cycles >> 16) & 0xFF);
    dataBuf[9] = (uint8_t)((cycles >> 24) & 0xFF);

    dataBuf[10] = 0;
    dataBuf[11] = 0;
    dataBuf[12] = 0;

    outEvt.capId = platform::capability::CAP_DIAGNOSTICS_HEALTH;
    outEvt.kind = platform::envelope::Kind::Evt;
    outEvt.msgId = 0x10; // ALERT
    outEvt.flags = 0;
    outEvt.hasSeq = false;
    outEvt.seq = 0;
    outEvt.data = dataBuf;
    outEvt.dataLen = 13;
    return true;
}


bool GrowBedNode::buildEventFactoryValidation(platform::envelope::Envelope& outEvt,
                         uint8_t* dataBuf, uint16_t dataMax,
                         uint32_t seq, bool pass, uint8_t failCode, uint8_t failStep,
                         uint32_t durationMs, uint32_t uptimeMs, uint32_t cycles) {
    if (!dataBuf || dataMax < 21) return false;

    // DATA:
    // 0..3: seq (u32)
    // 4: pass(1)/fail(0)
    // 5: failCode
    // 6: failStep
    // 7..10: durationMs (u32)
    // 11..14: uptimeMs (u32)
    // 15..18: cycles (u32)
    // 19..20: reserved
    dataBuf[0] = (uint8_t)(seq & 0xFF);
    dataBuf[1] = (uint8_t)((seq >> 8) & 0xFF);
    dataBuf[2] = (uint8_t)((seq >> 16) & 0xFF);
    dataBuf[3] = (uint8_t)((seq >> 24) & 0xFF);

    dataBuf[4] = pass ? 1 : 0;
    dataBuf[5] = failCode;
    dataBuf[6] = failStep;

    dataBuf[7]  = (uint8_t)(durationMs & 0xFF);
    dataBuf[8]  = (uint8_t)((durationMs >> 8) & 0xFF);
    dataBuf[9]  = (uint8_t)((durationMs >> 16) & 0xFF);
    dataBuf[10] = (uint8_t)((durationMs >> 24) & 0xFF);

    dataBuf[11] = (uint8_t)(uptimeMs & 0xFF);
    dataBuf[12] = (uint8_t)((uptimeMs >> 8) & 0xFF);
    dataBuf[13] = (uint8_t)((uptimeMs >> 16) & 0xFF);
    dataBuf[14] = (uint8_t)((uptimeMs >> 24) & 0xFF);

    dataBuf[15] = (uint8_t)(cycles & 0xFF);
    dataBuf[16] = (uint8_t)((cycles >> 8) & 0xFF);
    dataBuf[17] = (uint8_t)((cycles >> 16) & 0xFF);
    dataBuf[18] = (uint8_t)((cycles >> 24) & 0xFF);

    dataBuf[19] = 0;
    dataBuf[20] = 0;

    outEvt.capId = platform::capability::CAP_DIAGNOSTICS_HEALTH;
    outEvt.kind = platform::envelope::Kind::Evt;
    outEvt.msgId = 0x11; // FACTORY_VALIDATION
    outEvt.flags = 0;
    outEvt.hasSeq = false;
    outEvt.seq = 0;
    outEvt.data = dataBuf;
    outEvt.dataLen = 21;
    return true;
}

} // namespace product::growbed
