#include "stdafx.h"
#include "Memory.h"

Memory::Memory()
{
	cartridgeReader = new CartridgeReader();
}

Memory::~Memory()
{
	delete cartridgeReader;
}


std::int8_t Memory::readByte(std::int16_t pos)
{
	return cartridgeReader->readByte(pos);
}

void Memory::setByte(std::int16_t pos, std::int8_t val)
{
	cartridgeReader->setByte(pos, val);
}