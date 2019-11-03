#include <SerialTransfer.h>
#include <CPU.h>
#include <Memory.h>
#include <Joypad.h>

SerialTransfer::SerialTransfer(std::shared_ptr<spdlog::logger> _logger)
    : logger(_logger)
{
    transfer_enabled = false;

    register_sb = 0x00; // 0xFF01
    register_sc = 0x00; // 0xFF02
    transfer_bit = 0x01; // 0xFF01 shift bit 0, default: 1
    transfer_step = 0x00;

    transfer_clock_load = 0;
    transfer_clocks_accumulated = 0x0000;
}

SerialTransfer::~SerialTransfer()
{

}

void SerialTransfer::setByte(const uint16_t& addr, const uint8_t& val, const bool is_color_gb)
{
    switch (addr)
    {
        case 0xFF01:
        {
            register_sb = val;  // Load SB
            break;
        }

        case 0xFF02:
        {
            const bool prev_enabled = register_sc & BIT7;

            register_sc = val & 0x83;

            if (!prev_enabled &&
                (register_sc & BIT7))
            {   // Just enabled serial transfer
                transfer_enabled = true;

                // Reset accumulated clocks
                transfer_clocks_accumulated = 0;

                // Figure out transfer clock load
                if (!is_color_gb)
                {   // DMG
                    transfer_clock_load = CLOCK_SPEED / 8192;
                }
                else
                {   // CGB TODO
                    transfer_clock_load = CLOCK_SPEED / 8192;
                }

                // Try to not care about bit 0 External Clock/Internal CLock,
                // We'll default to both Internal Clock on their own
            }
            else if (prev_enabled &&
                    !(register_sc & BIT7))
            {   // Disabled serial transfer
                transfer_enabled = false;
                transfer_bit = 0x01;    // Reset last transfer bit
            }
            break;
        }
    }
}

uint8_t SerialTransfer::readByte(const uint16_t& addr) const
{
    switch (addr)
    {
        case 0xFF01: return register_sb;
        case 0xFF02: return register_sc;
        default:     return 0xFF;
    }
}

// Returns true on successful byte transfer, false otherwise
bool SerialTransfer::tick(const uint8_t& ticks)
{
    if (!transfer_enabled)
    {
        return false;
    }

    transfer_clocks_accumulated += ticks;

    if (transfer_clocks_accumulated >= transfer_clock_load)
    {
        // Send SB bit 7

        // Pull in transfer bit

        // Left shift SB by 1
        register_sb <<= 1;

        // Pad SB bit 0 with 1
        register_sb |= 0x01;

        // Set SB bit 0 to the current transfered bit
        register_sb |= (transfer_bit & 0x01);

        transfer_step++;

        // Trigger Serial interrupt if just received last bit of full byte
        // so the game can read in SB
        if (transfer_step > 7)
        {   // Received full byte, enable interrupt
            transfer_step = 0;
            return true;
        }
    }

    return false;
}

void SerialTransfer::setTransferBit(const uint8_t& _transfer_bit)
{
    transfer_bit = _transfer_bit & 0x01;
}