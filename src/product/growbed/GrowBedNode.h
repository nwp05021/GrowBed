#pragma once
#include <Arduino.h>
#include "../../platform/envelope/Envelope.h"

class MotionController;

namespace product::growbed {

class GrowBedNode {
public:
    void begin(MotionController* motion);

    bool handleCommand(const platform::envelope::Envelope& cmd,
                       platform::envelope::Envelope& outReply,
                       uint8_t* replyDataBuf, uint16_t replyDataMax);

    bool buildTelemetryBasic(platform::envelope::Envelope& outTel,
                             uint8_t* dataBuf, uint16_t dataMax);

private:
    MotionController* _motion {nullptr};
};

} // namespace product::growbed
