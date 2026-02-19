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

} // namespace product::growbed
