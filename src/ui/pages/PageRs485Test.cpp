#include "ui/pages/PageRs485Test.h"
#include <cstdio>

namespace growbed::ui::pages
{
    void PageRs485Test::render(uint32_t)
    {
        char buf[48];

        m_display.setTextSize(1);
        m_display.setTextColor(Color::kTextDim, Color::kBg);
        m_display.drawText(20, 42, "Status");
        m_display.setTextColor(m_model.rs485Ready ? Color::kOnIcon : Color::kDanger, Color::kBg);
        m_display.drawText(92, 42, m_model.rs485Status[0] ? m_model.rs485Status : "IDLE");

        std::snprintf(buf, sizeof(buf), "TX:%lu  RX:%lu  ERR:%lu",
                      static_cast<unsigned long>(m_model.rs485TxCount),
                      static_cast<unsigned long>(m_model.rs485RxCount),
                      static_cast<unsigned long>(m_model.rs485ErrorCount));
        m_display.setTextColor(Color::kText, Color::kBg);
        m_display.drawText(20, 68, buf);

        m_display.setTextColor(Color::kTextDim, Color::kBg);
        m_display.drawText(20, 96, "Last TX");
        m_display.setTextColor(Color::kText, Color::kBg);
        m_display.drawText(104, 96, m_model.rs485LastTx[0] ? m_model.rs485LastTx : "-");

        m_display.setTextColor(Color::kTextDim, Color::kBg);
        m_display.drawText(20, 122, "Last RX");
        m_display.setTextColor(Color::kText, Color::kBg);
        m_display.drawText(104, 122, m_model.rs485LastRx[0] ? m_model.rs485LastRx : "-");

        drawRow(128, "Send Test", "PING", m_model.rs485Cursor == 0, Color::kAccentTemp, 1);
        drawRow(158, "Clear Count", "CLR", m_model.rs485Cursor == 1, Color::kWarn, 1);
        drawRow(188, "Back", "Menu", m_model.rs485Cursor == 2, Color::kTextDim, 1);
    }
}
