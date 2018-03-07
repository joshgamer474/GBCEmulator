#pragma once

#ifndef MEMORY_H
#define MEMORY_H

#include <vector>
#include "spdlog/spdlog.h"

#define WORK_RAM_SIZE 0x1000

#define INTERRUPT_VBLANK 0x01
#define INTERRUPT_LCD_STATUS 0x02
#define INTERRUPT_TIMER 0x04
#define INTERRUPT_SERIAL 0x08
#define INTERRUPT_JOYPAD 0x10

#define TIMER_DIV_RATE 16384

class CartridgeReader;
class MBC;
class GPU;
class Joypad;

class Memory
{
private:


public:

	Memory();
	~Memory();

	CartridgeReader *cartridgeReader;
	MBC *mbc;
	GPU *gpu;
	Joypad *joypad;

	//char memoryMap[0xFFFF];

	unsigned char high_ram[0x7F];
	unsigned char gamepad;
	unsigned char timer[0x04];
	unsigned char audio[0x30];
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
	std::uint64_t prev_clock_div, curr_clock, prev_clock_tima;
	std::uint16_t clock_frequency;
	
	void initWorkRAM(bool isColorGB);
	void initROMBanks();
	void do_oam_dma_transfer(std::uint8_t start_address);
	void writeToTimerRegisters(std::uint16_t addr, std::uint8_t val);
	void updateTimer(std::uint64_t ticks, double clock_speed);

	std::uint8_t readByte(std::uint16_t pos);
	void setByte(std::uint16_t pos, std::uint8_t val);
};
#endif