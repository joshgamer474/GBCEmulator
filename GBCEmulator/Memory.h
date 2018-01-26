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

	unsigned char internal_work_ram[0x2000];
	unsigned char high_ram[0x7F];
	unsigned char io[0x80];
	unsigned char interrupt_register;

	int num_working_ram_banks;
	int curr_working_ram_bank;
	bool is_color_gb;
	std::vector<std::vector<unsigned char>> working_ram_banks;
	
	void initWorkRAM(bool isColorGB);

	std::uint8_t readByte(std::uint16_t pos);
	void setByte(std::uint16_t pos, std::uint8_t val);
};
#endif