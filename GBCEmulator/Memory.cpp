#include "stdafx.h"
#include "Memory.h"

Memory::Memory()
{
	cartridgeReader = new CartridgeReader();
	mbc = NULL;
	gpu = NULL;

	initWorkRAM(false);
}

Memory::~Memory()
{
	delete cartridgeReader;
}


void Memory::initWorkRAM(bool isColorGB)
{
	is_color_gb = isColorGB;

	if (is_color_gb)
		num_working_ram_banks = 8;
	else
		num_working_ram_banks = 2;

	working_ram_banks.resize(num_working_ram_banks, std::vector<unsigned char>(WORK_RAM_SIZE, 0));
}


std::uint8_t Memory::readByte(std::uint16_t pos)
{
	if (cartridgeReader->is_bios && pos < 256)
	{
		return cartridgeReader->bios[pos];
	}


	switch (pos & 0xF000)
	{
		/// ROM and RAM
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


		/// GPU RAM
	case 0x8000:
	case 0x9000:

		if ((pos & 0xFF00) < 0x9800)
		{
			// 0x8000 - 0x97FF : Tile RAM
			// 0x9800 - 0x9BFF : BG Map Data 1
			// 0x9C00 - 0x9FFF : BG Map Data 2
			return gpu->readByte(pos);
		}


		/// GB Work RAM, GPU Object Attribute Memory (OAM), I/O, High RAM, Interrupt enable reg
	case 0xC000:
	case 0xD000:
	case 0xE000:
	case 0xF000:

		if ((pos & 0xFF00) < 0xFE00)
		{
			// 0xC000 - 0xFDFF : Internal work RAM and (echo) Internal work RAM
			if ((pos & 0xF000) < 0xD000)
			{
				// 0xC000 - 0xCFFF
				return working_ram_banks[0][pos - 0xD000];
			}
			else if ((pos & 0xF000) < 0xE000)
			{
				// 0xD000 - 0xDFFF
				return working_ram_banks[curr_working_ram_bank][pos - 0xE000];
			}
			else
			{
				//0xE000 - 0xFDFF : (echo) Internal work RAM
				if ((pos & 0xF000) < 0xF000)
				{
					// 0xE000 - 0xEFFF
					return working_ram_banks[0][pos - 0xE000];
				}
				else
				{
					// 0xF000 - 0xFDFF
					return working_ram_banks[curr_working_ram_bank][pos - 0xF000];
				}
			}

		}
		else if ((pos & 0xFFF0) < 0xFEA0)
		{
			// 0xFE00 - 0xFE9F : Sprite RAM
			return gpu->readByte(pos);
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
			return io[pos - 0xFF00];
		}
		else if (pos < 0xFFFF)
		{
			// 0xFF80 - 0xFFFE : High RAM area
			return high_ram[pos - 0xFF80];
		}
		else if (pos == 0xFFFF)
		{
			// 0xFFFF : Interrupt Enable Register
			return interrupt_register;
		}
	
	default:
		printf("WARNING - Memory::readByte() doesn't handle address: %#06x\n", pos);
		return 0;
	}


	//return cartridgeReader->readByte(pos);
}

void Memory::setByte(std::uint16_t pos, std::uint8_t val)
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

		// VRAM
	case 0x8000:
	case 0x9000:
		gpu->setByte(pos, val);
		break;

		// Work RAM, echo Work RAM, GPU memory, I/O, Interrupts
	case 0xC000:
	case 0xD000:
	case 0xE000:
	case 0xF000:

		if ((pos & 0xF000) < 0xD000)
		{
			// 0xC000 - 0xCFFF : Internal Work RAM bank 0
			working_ram_banks[0][(pos - 0xC000)] = val;
		}
		else if ((pos & 0xF000) < 0xE000)
		{
			// 0xD000 - 0xDFFF : Internal Work RAM bank 1-7
			working_ram_banks[curr_working_ram_bank][(pos - 0xD000)] = val;
		}
		else if ((pos & 0xFF00) < 0xFE00)
		{
			// 0xE000 - 0xFDFF : (echo) Internal Work RAM of 0xC000 - 0xDDFF
			if ((pos - 0xE000) < 0x1000)
			{
				working_ram_banks[0][(pos - 0xE000)] = val;	// 0xC000 - 0xCFFF : (echo) Internal Work RAM bank 0
			}
			else
			{
				working_ram_banks[curr_working_ram_bank][(pos - 0xF000)] = val;	// 0xD000 - 0xDFFF : (echo) Internal Work RAM bank 1-7
			}
		}
		else if ((pos & 0xFFF0) < 0xFEA0)
		{
			// 0xFE00 - 0xFE9F : Sprite RAM
			gpu->setByte(pos, val);
		}
		else if ((pos & 0xFFF0) < 0xFF00)
		{
			// 0xFEA0 - 0xFEFF : Unused
			printf("WARNING - Memory::setByte() doesn't handle address: %#06x\n", pos);
		}
		else if ((pos & 0xFFF0) < 0xFF80)
		{
			// 0xFF00 - 0xFF7F : Hardware I/O
			io[pos - 0xFF00] = val;
		}
		else if (pos < 0xFFFF)
		{
			// 0xFF80 - 0xFFFE : High RAM area
			high_ram[pos - 0xFF80] = val;
		}
		else if (pos == 0xFFFF)
		{
			// 0xFFFF : Interrupt Enable Register
			interrupt_register = val;
		}
		else
		{
			printf("WARNING - Memory::setByte() doesn't handle address: %#06x\n", pos);
		}

		break;

	default:
		printf("WARNING - Memory::setByte() doesn't handle address: %#06x, val: %#04x\n", pos, val);

	}




	//cartridgeReader->setByte(pos, val);
}


void Memory::initROMBanks()
{
	std::uint64_t counter = 0;
	for (int i = 0; i < cartridgeReader->num_ROM_banks; i++)
	{
		mbc->romBanks[i] = std::vector<unsigned char>(cartridgeReader->romBuffer.begin() + (i * mbc->ROM_BANK_SIZE), cartridgeReader->romBuffer.begin() + ((i + 1)* mbc->ROM_BANK_SIZE));
		counter += mbc->ROM_BANK_SIZE;
	}
}