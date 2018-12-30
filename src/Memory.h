#pragma once

#ifndef MEMORY_H
#define MEMORY_H

#include <vector>
#include <spdlog/spdlog.h>

#define WORK_RAM_SIZE 0x1000

#define INTERRUPT_VBLANK 0x01
#define INTERRUPT_LCD_STATUS 0x02
#define INTERRUPT_TIMER 0x04
#define INTERRUPT_SERIAL 0x08
#define INTERRUPT_JOYPAD 0x10

#define TIMER_DIV_RATE 16384

enum TIMER_REG {
    DIV,    // 0xFF04
    TIMA,   // 0xFF05
    TMA,    // 0xFF06
    TAC     // 0xFF07
};

class CartridgeReader;
class MBC;
class GPU;
class Joypad;
class APU;

class Memory
{
public:
	Memory();
	~Memory();

    void reset();
	void initWorkRAM(bool isColorGB);
	void initROMBanks();
	void do_oam_dma_transfer(uint8_t start_address);
    void do_cgb_oam_dma_transfer(uint16_t start_address, uint16_t dest_address, uint8_t & hdma5);
    void do_cgb_h_blank_dma(uint8_t & hdma5);
    void writeToTimerRegisters(uint16_t addr, uint8_t val);
	void updateTimer(uint64_t ticks, double clock_speed);

	uint8_t readByte(uint16_t pos);
	void setByte(uint16_t pos, uint8_t val);

    std::shared_ptr<CartridgeReader> cartridgeReader;
    std::shared_ptr<MBC> mbc;
    std::shared_ptr<GPU> gpu;
    std::shared_ptr<Joypad> joypad;
    std::shared_ptr<APU> apu;

    bool cgb_perform_speed_switch;
    unsigned char cgb_speed_mode;
    unsigned char cgb_undoc_reg_ff6c;
    unsigned char cgb_undoc_regs[0xFF77 - 0xFF72];
	unsigned char high_ram[0x7F];
	unsigned char gamepad;
	unsigned char timer[0x04];
	unsigned char linkport[3];
	bool firstTen = false;
	std::string blargg = "";

	unsigned char interrupt_flag;
	unsigned char interrupt_enable;

	int num_working_ram_banks;
	int curr_working_ram_bank;
	bool is_color_gb;
	std::vector<std::vector<unsigned char>> working_ram_banks;
	std::shared_ptr<spdlog::logger> logger;

	// Timer variables
	bool timer_enabled;
	uint64_t prev_clock_div, curr_clock, prev_clock_tima;
	uint32_t clock_frequency;

private:
    uint16_t cgb_dma_start_addr_offset;
    uint16_t cgb_dma_start_addr;
    uint16_t cgb_dma_dest_addr;
};
#endif