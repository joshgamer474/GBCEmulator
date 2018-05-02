#include "stdafx.h"
#include "Memory.h"
#include "CartridgeReader.h"
#include "MBC.h"
#include "GPU.h"
#include "Joypad.h"
#include "Debug.h"
#include <string>

Memory::Memory()
{
	cartridgeReader = new CartridgeReader();
	mbc = NULL;
	gpu = NULL;
	joypad = NULL;

	timer_enabled = false;
	prev_clock_div = prev_clock_tima = curr_clock = 0;
	clock_frequency = 0;

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
			logger->warn("Memory::readByte() doesn't handle address: 0x{0:x}", pos);
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
				//logger->warn("Memory::readByte() doesn't handle address: 0x{0:x}", pos);
				//return 0xFF;
				return linkport[pos - 0xFF03];
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
				logger->warn("Memory::readByte() doesn't handle address: 0x{0:x}", pos);
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
				logger->warn("Memory::readByte() doesn't handle address: 0x{0:x}", pos);
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
		logger->warn("Memory::readByte() doesn't handle address: 0x{0:x}", pos);
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

		if (mbc->mbc_num != 0)
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
			logger->warn("Memory::setByte() doesn't handle address: 0x{0:x}, val: 0x{1:x}", pos, val);
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
				//logger->warn("Memory::setByte() doesn't handle address: 0x{0:x}, val: 0x{1:x}", pos, val);
				
				if (pos == 0xFF02 && val == 0x81)
				{
					if (linkport[0] == 10 && firstTen == false)
						firstTen = true;
					else if (linkport[0] == 10 && firstTen == true)
					{
						firstTen = false;
						logger->info(blargg.c_str());
						blargg = "";
					}

					//logger->info("{}", std::to_string(linkport[0]));
					blargg += linkport[0];
				}
				
				linkport[pos - 0xFF01] = val;
			}
			else if (pos < 0xFF08)
			{
				// 0xFF04 - 0xFF07 : Timer
				//timer[pos - 0xFF04] = val;
				writeToTimerRegisters(pos, val);
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
				logger->warn("Memory::setByte() doesn't handle address: 0x{0:x}, val: 0x{1:x}", pos, val);
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
				logger->warn("Memory::setByte() doesn't handle address: 0x{0:x}, val: 0x{1:x}", pos, val);
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
			logger->warn("Memory::setByte() doesn't handle address: 0x{0:x}, val: 0x{1:x}", pos, val);
		}

		break;

	default:
		logger->warn("Memory::setByte() doesn't handle address: 0x{0:x}, val: 0x{1:x}", pos, val);

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

void Memory::writeToTimerRegisters(std::uint16_t addr, std::uint8_t val)
{
	switch (addr)
	{
	case 0xFF04:
		timer[addr - 0xFF04] = 0x00;
		break;

	case 0xFF05: case 0xFF06:
		timer[addr - 0xFF04] = val;
		break;

	case 0xFF07:
		timer[addr - 0xFF04] = val;

		// Bits 0-1
		switch ((val & 0x03))
		{
		case 0:	clock_frequency = 4096;	break;	// Hz
		case 1: clock_frequency = 262144; break;
		case 2: clock_frequency = 65536; break;
		case 3: clock_frequency = 16384; break;
		}

		// Bit 2
		timer_enabled = (val & 0x04);
	}
}


void Memory::updateTimer(std::uint64_t ticks, double clock_speed)
{
	// Update 0xFF04
	std::uint8_t divider_reg = timer[0];
	std::uint16_t timer_counter = timer[1];
	curr_clock = ticks;
	if (curr_clock - prev_clock_div >= (clock_speed / TIMER_DIV_RATE))
	{
		divider_reg++;
		prev_clock_div = curr_clock;
		timer[0] = divider_reg;
	}

	// Update 0xFF05
	if (timer_enabled && curr_clock - prev_clock_tima >= clock_frequency)
	{
		timer_counter++;
		if (timer_counter > 0xFF)
		{
			timer_counter = timer[2];
			interrupt_flag |= 0x04;
		}
		prev_clock_tima = curr_clock;
		timer[1] = (timer_counter & 0xFF);
	}
}