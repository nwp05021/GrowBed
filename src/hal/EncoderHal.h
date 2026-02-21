#pragma once
#include <stdint.h>

class EncoderHal {
public:
    virtual ~EncoderHal() = default;

    virtual uint32_t millisNow() const = 0;

    virtual int readA() const = 0;
    virtual int readB() const = 0;
    virtual int readBtn() const = 0;

    virtual void attachABInterrupts(void (*isr)()) = 0;
    virtual void detachABInterrupts() = 0;

    virtual void enterCritical() = 0;
    virtual void exitCritical() = 0;
};