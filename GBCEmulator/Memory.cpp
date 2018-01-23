#include "stdafx.h"
#include "Memory.h"

Memory::Memory()
{
	cartridgeReader = new CartridgeReader();
	mbc = NULL;
}

Memory::~Memory()
{
	delete cartridgeReader;
}


std::int8_t Memory::readByte(std::int16_t pos)
{
	switch (pos & 0xF000)
	{
		// ROM and RAM
	case 0x0000:
	case 0x1000:
	case 0x2000:
	case 0x3000:
	case 0x4000:
	case 0x5000:
	case 0x6000:
	case 0x7000:
	case 0xA000:
	case 0xB000:
		return mbc->readByte(pos);


	case 0x8000:
	case 0x9000:

		/// TODO: GPU RAM
		if ((pos & 0xFF00) < 0x9800)
		{
			// 0x8000 - 0x97FF : Tile RAM
		}
		else if ((pos & 0xFF00) < 0x9C00)
		{
			// 0x9800 - 0x9BFF : BG Map Data 1
		}
		else
		{
			// 0x9C00 - 0x9FFF : BG Map Data 2
		}

		printf("WARNING - Memory::readByte() doesn't handle address: %#06x yet\n", pos);
		return 0;


	case 0xC000:
	case 0xD000:
	case 0xE000:
	case 0xF000:

		if ((pos & 0xFF00) < 0xFE00)
		{
			// 0xC000 - 0xFDFF : Internal work RAM and (echo) Internal work RAM

		}
		else if ((pos & 0xFFF0) < 0xFEA0)
		{
			// 0xFE00 - 0xFE9F : Sprite RAM
		}
		else if ((pos & 0xFFF0) < 0xFF00)
		{
			// 0xFEA0 - 0xFEFF : Unused
			printf("WARNING - Memory::readByte() doesn't handle address: %#06x\n", pos);
			return 0;
		}
		else if ((pos & 0xFFF0) < 0xFF80)
		{
			// 0xFF00 - 0xFF7F : Hardware I/O
		}
		else if (pos < 0xFFFF)
		{
			// 0xFF80 - 0xFFFE : High RAM area
		}
		else if (pos == 0xFFFF)
		{
			// 0xFFFF : Interrupt Enable Register
		}
		else
		{
			printf("WARNING - Memory::readByte() doesn't handle address: %#06x\n", pos);
		}

		printf("WARNING - Memory::readByte() doesn't handle address: %#06x yet\n", pos);
		return 0;
	
	default:
		printf("WARNING - Memory::readByte() doesn't handle address: %#06x\n", pos);
		return 0;
	}


	//return cartridgeReader->readByte(pos);
}

void Memory::setByte(std::int16_t pos, std::int8_t val)
{
	switch (pos & 0xF000)
	{
		// ROM and RAM
	case 0x0000:
	case 0x1000:
	case 0x2000:
	case 0x3000:
	case 0x4000:
	case 0x5000:
	case 0x6000:
	case 0x7000:
	case 0xA000:
	case 0xB000:
		mbc->setByte(pos, val);
		break;

	default:
		printf("WARNING - Memory::setByte() doesn't handle address: %#06x, val: %#04x\n", pos, val);

	}




	//cartridgeReader->setByte(pos, val);
}