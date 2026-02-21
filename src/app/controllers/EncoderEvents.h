#pragma once

struct EncoderEvents {
    int delta = 0;            // -1 / 0 / +1
    bool shortPress = false;
    bool longPress = false;
    bool veryLongPress = false;
};