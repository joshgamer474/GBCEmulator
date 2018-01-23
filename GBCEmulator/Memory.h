#pragma once

#ifndef MEMORY_H
#define MEMORY_H

#include "CartridgeReader.h"
#include "MBC.h"

class Memory
{
private:


public:

	Memory();
	~Memory();

	CartridgeReader *cartridgeReader;
	MBC *mbc;

	//char memoryMap[0xFFFF];

	unsigned char internal_work_ram[0x2000];
	unsigned char high_ram[0x7F];
	unsigned char io[0x80];
	unsigned char interrupt_register;

	std::int8_t readByte(std::int16_t pos);
	void setByte(std::int16_t pos, std::int8_t val);
};
#endif