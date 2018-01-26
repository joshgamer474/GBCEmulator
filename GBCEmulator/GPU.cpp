#include "stdafx.h"

#include "GPU.h"

GPU::GPU()
{
	is_color_gb = false;
	num_vram_banks = 1;
	curr_vram_bank = 0;

	vram_banks.resize(num_vram_banks, std::vector<unsigned char>(VRAM_SIZE, 0));
	object_attribute_memory.resize(OAM_SIZE);
}


GPU::~GPU()
{

}


std::uint8_t GPU::readByte(std::uint16_t pos)
{
	switch (pos & 0xF000)
	{
	case 0x8000:
	case 0x9000:

		return vram_banks[curr_vram_bank][pos - 0x8000];

	case 0xF000:

		// 0xFE00 - 0xFE9F : Sprite RAM (OAM)
		return object_attribute_memory[pos - 0xFE00];

	default:
		printf("WARNING - GPU::readByte() doesn't handle address: %#06x\n", pos);
		return -1;
	}
}


void GPU::setByte(std::uint16_t pos, std::uint8_t val)
{
	switch (pos & 0xF000)
	{
	case 0x8000:
	case 0x9000:

		vram_banks[curr_vram_bank][pos - 0x8000] = val;
		break;

	case 0xF000:

		// 0xFE00 - 0xFE9F : Sprite RAM (OAM)
		object_attribute_memory[pos - 0xFE00] = val;
		break;

	default:
		printf("WARNING - GPU::setByte() doesn't handle address: %#06x\n", pos);
	}
}
