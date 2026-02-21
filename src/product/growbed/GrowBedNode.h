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

    // Event: alert/fault notification to LineBed
    bool buildEventAlert(platform::envelope::Envelope& outEvt,
                         uint8_t* dataBuf, uint16_t dataMax,
                         uint8_t faultCode, uint32_t uptimeMs, uint32_t cycles);

    // Event: factory validation result
    bool buildEventFactoryValidation(platform::envelope::Envelope& outEvt,
                         uint8_t* dataBuf, uint16_t dataMax,
                         uint32_t seq, bool pass, uint8_t failCode, uint8_t failStep,
                         uint32_t durationMs, uint32_t uptimeMs, uint32_t cycles);

private:
    MotionController* _motion {nullptr};
};

} // namespace product::growbed
