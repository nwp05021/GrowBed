#pragma once

#include "devices/Rs485Driver.h"
#include "domain/RuntimeState.h"
#include <cstdint>

namespace growbed::modules
{
    class Rs485Module
    {
    public:
        Rs485Module(domain::RuntimeState& state, devices::Rs485Driver& driver)
            : m_state(state), m_driver(driver) {}

        bool init(const devices::Rs485Driver::Config& cfg);
        void tick(uint32_t nowMs);

    private:
        domain::RuntimeState& m_state;
        devices::Rs485Driver& m_driver;
        uint32_t m_handledRequestSeq = 0;
        uint8_t  m_rxLen = 0;

        void sendTest(uint32_t nowMs);
        void pollRx();
        void setStatus(const char* text);
    };
}
