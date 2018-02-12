#include "stdafx.h"
#include "Memory.h"
#include "CartridgeReader.h"
#include "MBC.h"
#include "GPU.h"
#include "Joypad.h"
#include "Debug.h"

Memory::Memory()
{
	cartridgeReader = new CartridgeReader();
	mbc = NULL;
	gpu = NULL;
	joypad = NULL;

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

	curr_working_ram_bank = 1;

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


		// 0x8000 - 0x97FF : Tile RAM
		// 0x9800 - 0x9BFF : BG Map Data 1
		// 0x9C00 - 0x9FFF : BG Map Data 2
		return gpu->readByte(pos);
		


		/// GB Work RAM, GPU Object Attribute Memory (OAM), Hardware I/O, High RAM, Interrupt enable reg
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
				return working_ram_banks[0][pos - 0xC000];
			}
			else if ((pos & 0xF000) < 0xE000)
			{
				// 0xD000 - 0xDFFF
				return working_ram_banks[curr_working_ram_bank][pos - 0xD000];
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
			return 0xFF;
		}
		else if ((pos & 0xFFF0) < 0xFF80)
		{
			// 0xFF00 - 0xFF7F : Hardware I/O
			
			
			if (pos == 0xFF00)
			{
				// 0xFF0 : Gamepad
				return joypad->get_joypad_byte();
			}
			else if (pos < 0xFF04)
			{
				/// TODO: handle Serial Data
				// 0xFF01 - 0xFF03 : Serial Data and Not referenced
				printf("WARNING - Memory::readByte() doesn't handle address: %#06x\n", pos);
				return 0;
			}
			else if (pos < 0xFF08)
			{
				// 0xFF04 - 0xFF07 : Timer
				return timer[pos - 0xFF04];
			}
			else if (pos == 0xFF0F)
			{
				// 0xFF0F : Interrupt Flag
				return interrupt_flag;
			}
			else if (pos < 0xFF10)
			{
				// 0xFF08 - 0xFF09 : Not referenced
				printf("WARNING - Memory::readByte() doesn't handle address: %#06x\n", pos);
				return 0xFF;
			}
			else if (pos < 0xFF40)
			{
				// 0xFF10 - 0xFF3F : Audio
				return audio[pos - 0xFF10];
			}
			else if (pos < 0xFF6C)
			{
				// 0xFF40 - 0xFF6B : GPU LCD
				return gpu->readByte(pos);
			}
			else
			{
				// 0xFF6C - 0xFF7F : Not referenced
				printf("WARNING - Memory::readByte() doesn't handle address: %#06x\n", pos);
				return 0xFF;
			}
		}
		else if (pos < 0xFFFF)
		{
			// 0xFF80 - 0xFFFE : High RAM area
			return high_ram[pos - 0xFF80];
		}
		else if (pos == 0xFFFF)
		{
			// 0xFFFF : Interrupt Enable Register
			/// TODO:
			/*
				Bit 0: V-Blank  Interrupt Enable  (INT 40h)  (1=Enable)
				 Bit 1: LCD STAT Interrupt Enable  (INT 48h)  (1=Enable)
				 Bit 2: Timer    Interrupt Enable  (INT 50h)  (1=Enable)
				 Bit 3: Serial   Interrupt Enable  (INT 58h)  (1=Enable)
				 Bit 4: Joypad   Interrupt Enable  (INT 60h)  (1=Enable)
			*/
			return interrupt_enable;
		}
	
	default:
		printf("WARNING - Memory::readByte() doesn't handle address: %#06x\n", pos);
		return 0xFF;
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
			printf("WARNING - Memory::setByte() doesn't handle address: %#06x, val: %#04x\n", pos, val);
		}
		else if ((pos & 0xFFF0) < 0xFF80)
		{
			// 0xFF00 - 0xFF7F : Hardware I/O
			
			if (pos == 0xFF00)
			{
				// 0xFF0 : Gamepad
				joypad->set_joypad_byte(val);
			}
			else if (pos < 0xFF04)
			{
				/// TODO: handle Serial Data
				// 0xFF01 - 0xFF03 : Serial Data and Not referenced
				printf("WARNING - Memory::setByte() doesn't handle address: %#06x, val: %#04x\n", pos, val);
			}
			else if (pos < 0xFF08)
			{
				// 0xFF04 - 0xFF07 : Timer
				timer[pos - 0xFF04] = val;
			}
			else if (pos == 0xFF0F)
			{
				// 0xFF0F : Interrupt Flag
				/// TODO:
				/*
				Bit 0: V-Blank  Interrupt Request (INT 40h)  (1=Request)
				Bit 1: LCD STAT Interrupt Request (INT 48h)  (1=Request)
				Bit 2: Timer    Interrupt Request (INT 50h)  (1=Request)
				Bit 3: Serial   Interrupt Request (INT 58h)  (1=Request)
				Bit 4: Joypad   Interrupt Request (INT 60h)  (1=Request)
				*/
				interrupt_flag = val;
			}
			else if (pos < 0xFF10)
			{
				// 0xFF08 - 0xFF09 : Not referenced
				printf("WARNING - Memory::setByte() doesn't handle address: %#06x, val: %#04x\n", pos, val);
			}
			else if (pos < 0xFF40)
			{
				// 0xFF10 - 0xFF3F : Audio
				audio[pos - 0xFF10] = val;
			}
			else if (pos < 0xFF6C)
			{
				// 0xFF40 - 0xFF6B : GPU LCD
				gpu->setByte(pos, val);
			}
			else
			{
				// 0xFF6C - 0xFF7F : Not referenced
				printf("WARNING - Memory::setByte() doesn't handle address: %#06x, val: %#04x\n", pos, val);
			}
		}
		else if (pos < 0xFFFF)
		{
			// 0xFF80 - 0xFFFE : High RAM area
			high_ram[pos - 0xFF80] = val;
		}
		else if (pos == 0xFFFF)
		{
			// 0xFFFF : Interrupt Enable Register
			/// TODO:
			/*
				 Bit 0: V-Blank  Interrupt Enable  (INT 40h)  (1=Enable)
			 Bit 1: LCD STAT Interrupt Enable  (INT 48h)  (1=Enable)
			 Bit 2: Timer    Interrupt Enable  (INT 50h)  (1=Enable)
			 Bit 3: Serial   Interrupt Enable  (INT 58h)  (1=Enable)
			 Bit 4: Joypad   Interrupt Enable  (INT 60h)  (1=Enable)
			*/
			interrupt_enable = val;
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

// Performs copying of ROM/RAM to GPU->OAM memory
void Memory::do_oam_dma_transfer(std::uint8_t start_address)
{
	std::uint16_t source_addr, dest_addr;
	std::uint8_t val;

	source_addr = (static_cast<std::uint16_t>(start_address) << 8);
	dest_addr = 0xFE00;

	// Copy memory from Source 0xZZ00 - 0xZZ9F to OAM memory (0xFE00 - 0xFE9F)
	for (dest_addr; dest_addr < 0xFEA0; dest_addr++, source_addr++)
	{
		val = readByte(source_addr);
		setByte(dest_addr, val);
	}
	
}