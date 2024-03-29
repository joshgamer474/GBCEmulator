#ifdef _WIN32
#include "stdafx.h"
#endif // _WIN32

#include <GBCEmulator.h>
#include <Memory.h>
#include <CartridgeReader.h>
#include <MBC.h>
#include <GPU.h>
#include <Joypad.h>
#include <APU.h>
#include <SerialTransfer.h>
#include "Debug.h"
#include <string>

Memory::Memory(std::shared_ptr<spdlog::logger> _logger,
    std::shared_ptr<CartridgeReader> _cartidgeReader,
    std::shared_ptr<MBC> _mbc,
    std::shared_ptr<GPU> _gpu,
    std::shared_ptr<Joypad> _joypad,
    std::shared_ptr<APU> _apu,
    std::shared_ptr<SerialTransfer> _serial_transfer,
    const bool force_cgb_mode)
    : logger(_logger)
    , cartridgeReader(_cartidgeReader)
    , mbc(_mbc)
    , gpu(_gpu)
    , joypad(_joypad)
    , apu(_apu)
    , serial_transfer(_serial_transfer)
{
	timer_enabled   = false;
	clock_frequency = 4096;
    clock_speed = 0;
    clock_div_accumulator = 0;
    clock_tima_accumulator = 0;

    interrupt_flag      = false;
    interrupt_enable    = false;

    cgb_speed_mode      = 0;
    cgb_undoc_reg_ff6c  = 0;
    cgb_perform_speed_switch = false;

	initWorkRAM(cartridgeReader->isColorGB() || force_cgb_mode);
    initROMBanks();
}

Memory::~Memory()
{
    reset();
    logger.reset();
}

Memory& Memory::operator=(const Memory& rhs)
{   // Copy from rhs
    timer_enabled           = rhs.timer_enabled;
    clock_frequency         = rhs.clock_frequency;
    clock_speed             = rhs.clock_speed;
    clock_div_accumulator   = rhs.clock_div_accumulator;
    clock_tima_accumulator  = rhs.clock_tima_accumulator;
    interrupt_flag          = rhs.interrupt_flag;
    interrupt_enable        = rhs.interrupt_enable;
    cgb_speed_mode          = rhs.cgb_speed_mode;
    cgb_undoc_reg_ff6c      = rhs.cgb_undoc_reg_ff6c;
    cgb_perform_speed_switch = rhs.cgb_perform_speed_switch;

    memcpy(cgb_undoc_regs, rhs.cgb_undoc_regs, 0xFF77 - 0xFF72);
    memcpy(high_ram, rhs.high_ram, 0x7F);
    gamepad = rhs.gamepad;
    memcpy(timer, rhs.timer, 4);
    memcpy(linkport, rhs.linkport, 3);
    firstTen = rhs.firstTen;
    blargg = rhs.blargg;

    is_color_gb             = rhs.is_color_gb;
    num_working_ram_banks   = rhs.num_working_ram_banks;
    curr_working_ram_bank   = rhs.curr_working_ram_bank;
    working_ram_banks       = rhs.working_ram_banks;

    return *this;
}

void Memory::reset()
{
    cartridgeReader.reset();
    mbc.reset();
    gpu.reset();
    joypad.reset();
    apu.reset();
}

void Memory::initWorkRAM(bool isColorGB)
{
    is_color_gb = isColorGB;

    if (is_color_gb)
    {
        num_working_ram_banks = 8;
    }
    else
    {
        num_working_ram_banks = 2;
    }

    curr_working_ram_bank = 1;

    working_ram_banks.resize(num_working_ram_banks, std::vector<unsigned char>(WORK_RAM_SIZE, 0));

    logger->info("Initializing memory, is_color_gb: {}, num_working_ram_banks: {}",
        is_color_gb,
        num_working_ram_banks);
}


std::uint8_t Memory::readByte(std::uint16_t pos, bool limit_access) const
{
	if (cartridgeReader->has_bios &&
        cartridgeReader->is_in_bios &&
        pos < 0x0100)
	{
        if (pos < cartridgeReader->bios.size())
        {
          return cartridgeReader->bios[pos];
        }
        else
        {
            return 0xFF;
        }
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
		return gpu->readByte(pos, limit_access);

		/// GB Work RAM, GPU Object Attribute Memory (OAM), Hardware I/O, High RAM, Interrupt enable reg
	case 0xC000:
	case 0xD000:
	case 0xE000:
	case 0xF000:

		if ((pos & 0xFF00) < 0xFE00)
		{   // 0xC000 - 0xFDFF : Internal work RAM and (echo) Internal work RAM
			if ((pos & 0xF000) < 0xD000)
			{   // 0xC000 - 0xCFFF
                if (pos == 0xC264 || pos == 0xC265)
                {
                    logger->info("Reading pos 0x{0:x}, val: 0x{1:x}",
                        pos,
                        working_ram_banks[0][pos - 0xC000]);
                }
				return working_ram_banks[0][pos - 0xC000];
			}
			else if ((pos & 0xF000) < 0xE000)
			{
                if (is_color_gb)
                {   // 0xD000 - 0xDFFF
                    if (curr_working_ram_bank == 0)
                    {
                        return working_ram_banks[1][pos - 0xD000];
                    }
                    return working_ram_banks[curr_working_ram_bank][pos - 0xD000];
                }
                else
                {   // Can only access Working RAM bank 1 in non color gb mode
                    return working_ram_banks[1][pos - 0xD000];
                }
			}
			else
			{   //0xE000 - 0xFDFF : (echo) Internal work RAM
				if ((pos & 0xF000) < 0xF000)
				{   // 0xE000 - 0xEFFF
					return working_ram_banks[0][pos - 0xE000];
				}
				else if (pos >= 0xF000 && is_color_gb)
				{   // 0xF000 - 0xFDFF
                    if (curr_working_ram_bank == 0)
                    {
                        return working_ram_banks[1][pos - 0xF000];
                    }
					return working_ram_banks[curr_working_ram_bank][pos - 0xF000];
				}
                else
                {   // Can only access Working RAM bank 1 in non color gb mode
                    return working_ram_banks[1][pos - 0xF000];
                }
			}

		}
		else if ((pos & 0xFFF0) < 0xFEA0)
		{
			// 0xFE00 - 0xFE9F : Sprite RAM
			return gpu->readByte(pos, limit_access);
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
				//return linkport[pos - 0xFF03];
                return serial_transfer->readByte(pos);
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
                return apu->readByte(pos);
			}
            else if (pos == 0xFF4D)
            {
                return cgb_speed_mode;
            }
			else if (pos < 0xFF6C)
			{
				// 0xFF40 - 0xFF6B : GPU LCD
				return gpu->readByte(pos, limit_access);
			}
            else if (pos == 0xFF6C && is_color_gb)
            {
                return cgb_undoc_reg_ff6c;
            }
            else if (pos == 0xFF70)
            {
                // 0xFF70 : WRAM select (CBG Only)
                return curr_working_ram_bank | 0xF8;    // Bits 3-7 are masked with 1s
            }
            else if (pos >= 0xFF72 && pos <= 0xFF77 && is_color_gb)
            {
                if (pos == 0xFF75)
                {
                    return 0x8F | cgb_undoc_regs[pos - 0xFF72];  // Only bits 4-6 are readable, rest are 1s
                }
                else if (pos >= 0xFF72 && pos <= 0xFF77)
                {
                    return cgb_undoc_regs[pos - 0xFF72];
                }
            }
			else
			{
				// 0xFF6C - 0xFF6F and 0xFF71 - 0xFF7F : Not referenced
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

void Memory::setByte(std::uint16_t pos, std::uint8_t val, bool limit_access)
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
		gpu->setByte(pos, val, limit_access);
		break;

		// Work RAM, echo Work RAM, GPU memory, I/O, Interrupts
	case 0xC000:
	case 0xD000:
	case 0xE000:
	case 0xF000:

		if ((pos & 0xF000) < 0xD000)
		{   // 0xC000 - 0xCFFF : Internal Work RAM bank 0
            if (pos == 0xC264 || pos == 0xC265)
            {
                logger->info("Writing pos: 0x{0:x}, val: 0x{1:x}",
                    pos,
                    val);
            }
			working_ram_banks[0][pos - 0xC000] = val;
		}
		else if ((pos & 0xF000) < 0xE000)
		{   // 0xD000 - 0xDFFF : Internal Work RAM bank 1-7
            if (is_color_gb)
            {
                if (curr_working_ram_bank == 0)
                {
                    working_ram_banks[1][pos - 0xD000] = val;
                }
                else
                {
                    working_ram_banks[curr_working_ram_bank][pos - 0xD000] = val;
                }
            }
            else
            {   // Only Work RAM bank 1 is available in non color gb mode
                working_ram_banks[1][pos - 0xD000] = val;
            }
		}
		else if ((pos & 0xFF00) < 0xFE00)
		{   // 0xE000 - 0xFDFF : (echo) Internal Work RAM of 0xC000 - 0xDDFF
			if ((pos - 0xE000) < 0x1000)
			{   // 0xC000 - 0xCFFF : (echo) Internal Work RAM bank 0
				working_ram_banks[0][pos - 0xE000] = val;
			}
			else if (pos >= 0xD000 && pos <= 0xDFFF)
			{   // 0xD000 - 0xDFFF : (echo) Internal Work RAM bank 1-7
                if (is_color_gb)
                {
                    if (curr_working_ram_bank == 0)
                    {
                        working_ram_banks[1][pos - 0xF000] = val;
                    }
                    else
                    {
                        working_ram_banks[curr_working_ram_bank][pos - 0xF000] = val;
                    }
                }
                else
                {   // Only Work RAM bank 1 is available in non color gb mode
                    working_ram_banks[1][pos - 0xF000] = val;
                }
			}
		}
		else if ((pos & 0xFFF0) < 0xFEA0)
		{   // 0xFE00 - 0xFE9F : Sprite RAM
			gpu->setByte(pos, val, limit_access);
		}
		else if ((pos & 0xFFF0) < 0xFF00)
		{   // 0xFEA0 - 0xFEFF : Unused
			logger->warn("Memory::setByte() doesn't handle address: 0x{0:x}, val: 0x{1:x}", pos, val);
		}
		else if ((pos & 0xFFF0) < 0xFF80)
		{   // 0xFF00 - 0xFF7F : Hardware I/O
			
			if (pos == 0xFF00)
			{   // 0xFF00 : Gamepad
				joypad->set_joypad_byte(val & 0xF0);    // First four bits are read-only
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

                if (pos == 0xFF01 || pos == 0xFF02)
                {
                    serial_transfer->setByte(pos, val, is_color_gb);
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
				interrupt_flag = 0xE0 | val;
                //interrupt_flag = val;
			}
			else if (pos < 0xFF10)
			{
				// 0xFF08 - 0xFF09 : Not referenced
				logger->warn("Memory::setByte() doesn't handle address: 0x{0:x}, val: 0x{1:x}", pos, val);
			}
			else if (pos < 0xFF40)
			{
				// 0xFF10 - 0xFF3F : Audio
                apu->setByte(pos, val);
			}
            else if (pos == 0xFF4D)
            {
                if ((val & 0x01) && (cgb_speed_mode & 0x01) == 0)
                {
                    gpu->setByte(0xFF40, gpu->readByte(0xFF40, limit_access) & 0x7F); // Disable LCD 
                    cgb_perform_speed_switch = true;
                }
                cgb_speed_mode = val & 0x01;    // Only bit 0 is writable
            }
			else if (pos < 0xFF6C)
			{
				// 0xFF40 - 0xFF6B : GPU LCD
				gpu->setByte(pos, val, limit_access);
			}
            else if (pos == 0xFF6C && is_color_gb)
            {
                cgb_undoc_reg_ff6c = 0xFE | val;
            }
            else if (pos == 0xFF70 && is_color_gb)
            {
                // 0xFF70 : WRAM select (CBG Only)
                if (val == 0x00)
                {   // Cannot select WRAM bank 00
                    val = 0x01;
                }

                curr_working_ram_bank = val & 0x07;
            }
            else if (pos >= 0xFF72 && pos <= 0xFF77 && is_color_gb)
            {
                if (pos == 0xFF75)
                {
                    cgb_undoc_regs[pos - 0xFF72] = 0x8F | val;  // Only bits 4-6 are writable, rest are 1s
                }
                else
                {
                    cgb_undoc_regs[pos - 0xFF72] = val;
                }
            }
			else
			{
                // 0xFF6C - 0xFF6F and 0xFF71 - 0xFF7F : Not referenced
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
            //interrupt_enable = 0xE0 | val;
            logger->info("Writing 0x{0:x} to interrupt_enable", val);
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
}


void Memory::initROMBanks()
{
	std::uint64_t counter = 0;
	for (int i = 0; i < cartridgeReader->num_ROM_banks; i++)
	{
		mbc->romBanks[i] = std::vector<unsigned char>(cartridgeReader->romBuffer.begin() + (i * mbc->ROM_BANK_SIZE), cartridgeReader->romBuffer.begin() + ((i + 1)* mbc->ROM_BANK_SIZE));
		counter += mbc->ROM_BANK_SIZE;
	}

    // Free cartridgeReader from holding ROM in memory
    // since it's now in the emulator's ROM banks
    cartridgeReader->freeRom();
}

// Performs copying of ROM/RAM to GPU->OAM memory
void Memory::do_oam_dma_transfer(std::uint8_t start_address)
{
	std::uint16_t source_addr, dest_addr;
	std::uint8_t val;

	source_addr = (static_cast<std::uint16_t>(start_address) << 8);
	dest_addr = 0xFE00;

    if (gpu->gpu_mode == GPU::GPU_MODE::GPU_MODE_OAM ||
        gpu->gpu_mode == GPU::GPU_MODE::GPU_MODE_VRAM)
    {
        logger->warn("Returning on OAM DMA, gpu_mode: {}", gpu->gpu_mode);
        return;
    }

    logger->info("Performing OAM DMA from starting source: {0:x}", source_addr);

	// Copy memory from Source 0xZZ00 - 0xZZ9F to OAM memory (0xFE00 - 0xFE9F)
	for (dest_addr; dest_addr < 0xFEA0; dest_addr++, source_addr++)
	{
		val = readByte(source_addr, false);
		setByte(dest_addr, val, false);
	}
	
    gpu->bg_tiles_updated = true;
}

// Performs copying of ROM/RAM to GPU->OAM memory
void Memory::do_cgb_oam_dma_transfer(uint8_t & hdma1, uint8_t & hdma2, uint8_t & hdma3, uint8_t & hdma4, uint8_t & hdma5)
{
    uint8_t val;
    uint16_t startAddress = (static_cast<uint16_t>(hdma1) << 8) | hdma2;
    uint16_t destAddress = (static_cast<uint16_t>(hdma3) << 8) | hdma4;

    // Should be between range 0x0000-0x7FF0 or 0xA000-0xDFF0
    startAddress &= 0xFFF0;    // Lower 4-bits are treated as 0s

    // Should be between range 0x8000-0x9FF0
    destAddress &= 0x1FF0;     // Only bits 12-4 are respected
    destAddress |= 0x8000;     // Dest is at a minimum 0x8000

    uint16_t transfer_length = hdma5 & 0x7F;
    bool h_blank_dma        = hdma5 & 0x80;

    // Transfer length is divided by 0x10, minus 1
    transfer_length = (transfer_length + 1) * 0x10;

    logger->info("Performing HDMA5 DMA from starting source: {0:x}, dest source: {1:x}, length: {2:x}, DMA type: {3:b}",
        startAddress,
        destAddress,
        transfer_length,
        h_blank_dma);

    if (h_blank_dma == false)
    {
        // Copy memory from Source address to Dest address
        for (uint16_t i = 0; i < transfer_length; i++)
        {
            val = readByte(startAddress++, false);
            setByte(destAddress++, val, false);
        }

        hdma5 = 0xFF;
        gpu->bg_tiles_updated = true;
        gpu->cgb_dma_in_progress = false;
    }
    else
    {
        hdma5 &= 0x7F;  // Set BIT7 == 0 to tell games that transfer is active
        gpu->cgb_dma_hblank_in_progress = true;
        logger->info("Going to do H-Blank DMA transfer from addr: 0x{0:x} to addr: 0x{1:x}, length: 0x{2:x}",
            startAddress,
            destAddress,
            transfer_length);
    }

    // Update HDMA registers
    hdma1 = startAddress >> 8;
    hdma2 = startAddress & 0x00FF;
    hdma3 = destAddress >> 8;
    hdma4 = destAddress & 0x00FF;
}

void Memory::do_cgb_h_blank_dma(uint8_t & hdma1, uint8_t & hdma2, uint8_t & hdma3, uint8_t & hdma4, uint8_t & hdma5)
{
    uint8_t val = 0;
    uint8_t length = hdma5 & 0x7F;
    uint16_t numBytesToTransfer = (length + 1) * 0x10;
    uint16_t startAddress = (static_cast<uint16_t>(hdma1) << 8) | hdma2;
    uint16_t destAddress = (static_cast<uint16_t>(hdma3) << 8) | hdma4;

    logger->info("Doing H-Blank DMA transfer from addr: 0x{0:x} to addr: 0x{1:x}, length: 0x{2:x}",
        startAddress,
        destAddress,
        numBytesToTransfer);

    for (uint16_t i = 0; i < 0x10; i++)
    {
        val = readByte(startAddress++, false);
        setByte(destAddress++, val, false);
        numBytesToTransfer--;
    }

    // Calculate new length
    length = (numBytesToTransfer / 0x10) - 1;

    // Update HDMA registers
    hdma1 = startAddress >> 8;
    hdma2 = startAddress & 0x00FF;
    hdma3 = destAddress >> 8;
    hdma4 = destAddress & 0x00FF;

    // Update HDMA5 length remaining
    hdma5 = length;

    if (hdma5 == 0xFF)
    {   // Transfer has completed, set HDMA5 to 0xFF
        logger->info("H-Blank DMA transfer complete");
        gpu->bg_tiles_updated = true;
        gpu->cgb_dma_in_progress = false;
        gpu->cgb_dma_hblank_in_progress = false;
    }
}

void Memory::writeToTimerRegisters(std::uint16_t addr, std::uint8_t val)
{
	const uint32_t old_clock_frequency = clock_frequency;
    bool old_timer_enabled = timer_enabled;

	switch (addr)
	{
	case 0xFF04: timer[DIV] = 0x00; break;  // Divider Register
    case 0xFF05: timer[TIMA] = val; break;  // Timer Counter
    case 0xFF06: timer[TMA] = val; break;   // Timer Modulo
	case 0xFF07:                            // Timer Control
        timer[TAC] = val;

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

		// Reset timer when TIMA clock_frequncy gets updated or when Timer gets enabled
		if (old_clock_frequency != clock_frequency ||
            (old_timer_enabled == false && timer_enabled == true))
		{
            clock_div_accumulator = 0;
            clock_tima_accumulator = 0;
            updateTimerRates();
		}
        updateTimerRates();
	}
}

void Memory::updateTimerRates()
{
    clock_div_rate  = static_cast<uint32_t>((CLOCK_SPEED * 1.0) / TIMER_DIV_RATE);
    clock_tima_rate = static_cast<uint32_t>((CLOCK_SPEED * 1.0) / clock_frequency);
}

void Memory::updateTimer(const uint8_t & ticks, const uint32_t & clockSpeed)
{
    // Tick Serial Transfer
    // if (serial_transfer &&
    //     serial_transfer->tick(ticks))
    // {   // Signal Serial data interrupt
    //     interrupt_flag |= INTERRUPT_SERIAL;
    // }

	uint8_t & divider_reg       = timer[DIV];
	uint8_t & timer_counter     = timer[TIMA];

    clock_div_accumulator += ticks;
    clock_tima_accumulator += ticks;

    if (clock_speed != clockSpeed)
    {
        clock_speed = clockSpeed;
        updateTimerRates();
    }

    while (clock_div_accumulator >= clock_div_rate)
    {   // Increment 0xFF04 - DIV
        divider_reg++;
        clock_div_accumulator -= clock_div_rate;
        //clock_div_accumulator = 0;
    }

    // Update 0xFF05 - TIMA
    //while (timer_enabled && clock_tima_diff >= clock_tima_rate)
    if (timer_enabled &&
        clock_tima_accumulator >= clock_tima_rate)
    {
        timer_counter++;
        if (timer_counter == 0x00)
        {   // timer_counter overflowed, reload it with TMA
            timer_counter = timer[TMA];
            interrupt_flag |= INTERRUPT_TIMER;
        }
        clock_tima_accumulator -= clock_tima_rate;
    }
}

// Sets registers to values after running through boot up ROM
void Memory::initGBPowerOn()
{
    setByte(0xFF05, 0x00); // TIMA
    setByte(0xFF06, 0x00); // TMA
    setByte(0xFF07, 0x00); // TAC
    setByte(0xFF10, 0x80); // NR10
    setByte(0xFF11, 0xBF); // NR11
    setByte(0xFF12, 0xF3); // NR12
    setByte(0xFF14, 0xBF); // NR14
    setByte(0xFF16, 0x3F); // NR21
    setByte(0xFF17, 0x00); // NR22
    setByte(0xFF19, 0xBF); // NR24
    setByte(0xFF1A, 0x7F); // NR30
    setByte(0xFF1B, 0xFF); // NR31
    setByte(0xFF1C, 0x9F); // NR32
    setByte(0xFF1E, 0xBF); // NR33
    setByte(0xFF20, 0xFF); // NR41
    setByte(0xFF21, 0x00); // NR42
    setByte(0xFF22, 0x00); // NR43
    setByte(0xFF23, 0xBF); // NR44
    setByte(0xFF24, 0x77); // NR50
    setByte(0xFF25, 0xF3); // NR51
    setByte(0xFF26, 0xF1); // NR52 // 0xF1 GB, 0xF0 SGB 
    setByte(0xFF40, 0x91); // LCDC
    setByte(0xFF42, 0x00); // SCY
    setByte(0xFF43, 0x00); // SCX
    setByte(0xFF45, 0x00); // LYC
    setByte(0xFF47, 0xFC); // BGP
    setByte(0xFF48, 0xFF); // OBP0
    setByte(0xFF49, 0xFF); // OBP1
    setByte(0xFF4A, 0x00); // WY
    setByte(0xFF4B, 0x00); // WX
    setByte(0xFFFF, 0x00); // IE
}