#pragma once

#ifndef MEMORY_H
#define MEMORY_H

#include "CartridgeReader.h"
#include "MBC.h"
#include "GPU.h"

#define WORK_RAM_SIZE 0x1000

class Memory
{
private:


public:

	Memory();
	~Memory();

	CartridgeReader *cartridgeReader;
	MBC *mbc;
	GPU *gpu;

	//char memoryMap[0xFFFF];

	unsigned char high_ram[0x7F];
	unsigned char gamepad;
	unsigned char timer[0x04];
	unsigned char audio[0x30];

	unsigned char interrupt_flag;
	unsigned char interrupt_enable;

	int num_working_ram_banks;
	int curr_working_ram_bank;
	bool is_color_gb;
	std::vector<std::vector<unsigned char>> working_ram_banks;
	
	void initWorkRAM(bool isColorGB);

	void initROMBanks();

	std::uint8_t readByte(std::uint16_t pos);
	void setByte(std::uint16_t pos, std::uint8_t val);
};
#endif