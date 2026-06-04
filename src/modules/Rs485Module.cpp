#include "modules/Rs485Module.h"
#include <cstdio>
#include <cstring>

namespace growbed::modules
{
    bool Rs485Module::init(const devices::Rs485Driver::Config& cfg)
    {
        const bool ok = m_driver.init(cfg);
        m_state.rs485Ready = ok;
        setStatus(ok ? "READY" : "INIT FAIL");
        return ok;
    }

    void Rs485Module::tick(uint32_t nowMs)
    {
        pollRx();

        if (m_state.rs485TestRequestSeq != m_handledRequestSeq) {
            m_handledRequestSeq = m_state.rs485TestRequestSeq;
            sendTest(nowMs);
        }
    }

    void Rs485Module::sendTest(uint32_t nowMs)
    {
        if (!m_driver.isReady()) {
            ++m_state.rs485ErrorCount;
            setStatus("NOT READY");
            return;
        }

        char msg[32];
        std::snprintf(msg, sizeof(msg), "PING %lu\r\n",
                      static_cast<unsigned long>(m_state.rs485TxCount + 1U));

        m_driver.flushInput();
        const size_t written = m_driver.writeText(msg);
        if (written == 0U) {
            ++m_state.rs485ErrorCount;
            setStatus("TX FAIL");
            return;
        }

        ++m_state.rs485TxCount;
        m_state.rs485LastTestMs = nowMs;
        std::snprintf(m_state.rs485LastTx, sizeof(m_state.rs485LastTx), "%s", msg);
        setStatus("TX SENT");
    }

    void Rs485Module::pollRx()
    {
        bool received = false;
        while (m_driver.available() > 0) {
            const int c = m_driver.read();
            if (c < 0) break;
            received = true;

            if (c == '\r') continue;
            if (c == '\n') {
                if (m_rxLen > 0U) {
                    m_state.rs485LastRx[m_rxLen] = '\0';
                    m_rxLen = 0U;
                }
                continue;
            }

            if (m_rxLen < sizeof(m_state.rs485LastRx) - 1U) {
                m_state.rs485LastRx[m_rxLen++] = static_cast<char>(c);
                m_state.rs485LastRx[m_rxLen] = '\0';
            } else {
                m_rxLen = 0U;
                ++m_state.rs485ErrorCount;
                setStatus("RX OVERFLOW");
            }
        }

        if (received) {
            ++m_state.rs485RxCount;
            setStatus("RX OK");
        }
    }

    void Rs485Module::setStatus(const char* text)
    {
        std::snprintf(m_state.rs485Status, sizeof(m_state.rs485Status), "%s", text ? text : "");
    }
}
