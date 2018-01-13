#pragma once

#ifndef MEMORY_H
#define MEMORY_H

#include "CartridgeReader.h"

class Memory
{
private:


public:

	Memory();
	~Memory();

	CartridgeReader *cartridgeReader;

	//char memoryMap[0xFFFF];

	std::int8_t readByte(std::int16_t pos);
	void setByte(std::int16_t pos, std::int8_t val);
};
#endif