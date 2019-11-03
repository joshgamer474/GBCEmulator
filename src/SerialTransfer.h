#ifndef SRC_SERIALTRANSFER_H
#define SRC_SERIALTRANSFER_H

#include <stdint.h>
#include <memory>
#include <spdlog/spdlog.h>

class SerialTransfer
{
public:
    SerialTransfer(std::shared_ptr<spdlog::logger> logger);
    virtual ~SerialTransfer();

    void setByte(const uint16_t& addr, const uint8_t& val, const bool is_color_gb);
    uint8_t readByte(const uint16_t& addr) const;
    bool tick(const uint8_t& ticks);
    void setTransferBit(const uint8_t& transfer_bit);

private:
    std::shared_ptr<spdlog::logger> logger;
    bool transfer_enabled;
    uint8_t register_sb;    // 0xFF01
    uint8_t register_sc;    // 0xFF02
    uint8_t transfer_bit;   // 0xFF01 shift bit 0, default: 1
    uint8_t transfer_step;
    uint16_t transfer_clock_load;
    uint16_t transfer_clocks_accumulated;
};

#endif // SRC_SERIALTRANSFER_H