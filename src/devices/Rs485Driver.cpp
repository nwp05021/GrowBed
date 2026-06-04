#include "devices/Rs485Driver.h"
#include <cstring>

namespace growbed::devices
{
    bool Rs485Driver::init(const Config& cfg)
    {
        m_cfg = cfg;

        if (m_cfg.pinDe >= 0) {
            pinMode(m_cfg.pinDe, OUTPUT);
            digitalWrite(m_cfg.pinDe, LOW);
        }
        if (m_cfg.pinRe >= 0) {
            pinMode(m_cfg.pinRe, OUTPUT);
            digitalWrite(m_cfg.pinRe, LOW);
        }

        if (m_cfg.pinTx >= 0) m_serial.setTX(m_cfg.pinTx);
        if (m_cfg.pinRx >= 0) m_serial.setRX(m_cfg.pinRx);
        m_serial.begin(m_cfg.baud);

        m_ready = true;
        return true;
    }

    size_t Rs485Driver::write(const uint8_t* data, size_t len)
    {
        if (!m_ready || !data || len == 0U) return 0U;

        setTransmitMode(true);
        delayMicroseconds(80);
        const size_t written = m_serial.write(data, len);
        m_serial.flush();
        delayMicroseconds(80);
        setTransmitMode(false);
        return written;
    }

    size_t Rs485Driver::writeText(const char* text)
    {
        if (!text) return 0U;
        return write(reinterpret_cast<const uint8_t*>(text), std::strlen(text));
    }

    int Rs485Driver::available() const
    {
        return m_ready ? m_serial.available() : 0;
    }

    int Rs485Driver::read()
    {
        return m_ready ? m_serial.read() : -1;
    }

    void Rs485Driver::flushInput()
    {
        while (available() > 0) {
            (void)read();
        }
    }

    void Rs485Driver::setTransmitMode(bool tx)
    {
        if (m_cfg.pinRe >= 0) digitalWrite(m_cfg.pinRe, tx ? HIGH : LOW);
        if (m_cfg.pinDe >= 0) digitalWrite(m_cfg.pinDe, tx ? HIGH : LOW);
    }
}
