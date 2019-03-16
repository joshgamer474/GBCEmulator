#ifdef _WIN32
#include "stdafx.h"
#endif // _WIN32

#include "MBC.h"
#include "Debug.h"
#include <fstream>
#include <chrono>
#include <ctime>
#include <exception>

MBC::MBC(int mbcNum, int numROMBanks, int numRAMBanks, std::shared_ptr<spdlog::logger> _logger)
{
    rom_banking_mode = true;
    ram_banking_mode = false;
    external_ram_enabled = false;
    curr_rom_bank = 1;
    curr_ram_bank = 1;

    num_rom_banks = numROMBanks;
    num_ram_banks = numRAMBanks;

    romBanks.resize(num_rom_banks, std::vector<unsigned char>(ROM_BANK_SIZE, 0));
    ramBanks.resize(num_ram_banks, std::vector<unsigned char>(RAM_BANK_SIZE, 0));

    mbc_num = mbcNum;
    mbc_type = static_cast<MBC_Type>(mbcNum);

    switch (mbc_num)
    {
    case MBC1:  MBC1_init(); break;
    case MBC2:  MBC2_init(); break;
    case MBC3:  MBC3_init(); break;
    case MBC5:  MBC5_init(); break;
    case MBC6:  MBC6_init(); break;
    case MBC7:  MBC7_init(); break;
    case HuC1:  HuC1_init(); break;
    case HuC3:  HuC3_init(); break;
    case MMM01: MMM01_init(); break;
    case TAMA5: TAMA5_init(); break;
    }

    logger = _logger;

    logger->info("Using MBC: {}", mbc_num);
}


MBC::~MBC()
{
    logger.reset();
}

void MBC::MBC1_init()
{
	setFromTo(&rom_from_to, 0x4000, 0x7FFF);
	setFromTo(&ram_from_to, 0xA000, 0xBFFF);
}


/*
	16 ROM banks
	Doesn't support external RAM, includes 512 4-bits of built-in RAM; only the lower 4 bits of the bytes in this memory are used
*/
void MBC::MBC2_init()
{
	setFromTo(&rom_from_to, 0x4000, 0x7FFF);
	setFromTo(&ram_from_to, 0xA000, 0xA1FF);
}

void MBC::MBC3_init()
{
    rtcRegisters.resize(0x0C - 0x08);

	setFromTo(&rom_from_to, 0x4000, 0x7FFF);
	setFromTo(&ram_from_to, 0xA000, 0xBFFF);
}

void MBC::MBC5_init()
{
    setFromTo(&rom_from_to, 0x4000, 0x7FFF);
    setFromTo(&ram_from_to, 0xA000, 0xBFFF);
}


void MBC::MBC6_init()
{
    throw std::runtime_error("MBC6 not yet supported!");
}

void MBC::MBC7_init()
{
    throw std::runtime_error("MBC7 not yet supported!");
}

void MBC::HuC1_init()
{
    throw std::runtime_error("HuC1 not yet supported!");
}

void MBC::HuC3_init()
{
    throw std::runtime_error("HuC3 not yet supported!");
}

void MBC::MMM01_init()
{
    throw std::runtime_error("MMM01 not yet supported!");
}

void MBC::TAMA5_init()
{
    throw std::runtime_error("TAMA5 not yet supported!");
}


void MBC::setFromTo(From_To *ft, int start, int end)
{
	ft->start = start;
	ft->end = end;
}


std::uint8_t MBC::readByte(std::uint16_t pos)
{
	switch (pos & 0xF000)
	{
		// ROM 00
	case 0x0000:
	case 0x1000:
	case 0x2000:
	case 0x3000:
		return romBanks[0][pos];

		// ROM 01 - N
	case 0x4000:
	case 0x5000:
	case 0x6000:
	case 0x7000:
        if (mbc_num == 1)
        {
            if (ram_banking_mode)
            {   // Only ROM banks 0x00-0x1F can be used during RAM banking mode
                return romBanks[curr_rom_bank % 0x1F][pos - 0x4000];
            }
        }
        
        // All ROM banks can be accessed
        return romBanks[curr_rom_bank % num_rom_banks][pos - 0x4000];

		// External RAM
	case 0xA000:
	case 0xB000:

        if (external_ram_enabled == false)
        {   // Cannot read/write to external RAM until this is enabled
            return 0xFF;
        }

        if (mbc_num == 1)
        {
            if (rom_banking_mode)
            {   // Only RAM bank 0x00 is accessible in ROM banking mode
                return ramBanks[0][pos - 0xA000];
            }
            else if (ram_banking_mode)
            {   // ram_banking_mode == true
                return ramBanks[curr_ram_bank % num_ram_banks][pos - 0xA000];
            }
            else
            {
                logger->warn("How'd you get here??");
                return 0xFF;
            }
        }
        else if (mbc_num == 3)
        {
            if (curr_ram_bank <= 0x03)
            {
                return ramBanks[curr_ram_bank % num_ram_banks][pos - 0xA000];
            }
            else if (curr_ram_bank >= 0x08 && curr_ram_bank <= 0x0C)
            {
                return rtcRegisters[curr_ram_bank - 0x08];
            }
        }
        else
        {
            return ramBanks[curr_ram_bank % num_ram_banks][pos - 0xA000];
        }

	default:
		logger->warn("MBC::readByte() used address: 0x{0:x}", pos);
		return 0;
	}
}


void MBC::setByte(std::uint16_t pos, std::uint8_t val)
{
	switch (pos & 0xF000)
	{
		// ROM 00
	case 0x0000:
	case 0x1000:

        // 0x0000 - 0x1FFF : Set RAM enable
        if ((val & 0x0F) == 0x0A)
        {
            external_ram_enabled = true;

            if (mbc_num == 3)
            {
                rtc_timer_enabled = true;
            }
        }
        else if (val == 0)
        {
            external_ram_enabled = false;
            rtc_timer_enabled = false;
        }

        break;

	case 0x2000:
	case 0x3000:

        if (val == 0x00 && mbc_num != 5)
        {   // Any attempts to select ROM bank 00 will be set to ROM bank 01 (unless MBC5)
            val = 0x01;
        }

		// 0x2000 - 0x3FFF : Set ROM bank number
        if (mbc_num == 1)
        {   // Set lower 5 bits of ROM bank number
            curr_rom_bank = (curr_rom_bank & 0x60) | (val & 0x1F);
        }
        else if (mbc_num == 2 && (pos & 0x0100) > 0)
        {   // Set lower 5 bits of ROM bank number
            curr_rom_bank = (curr_rom_bank & 0x60) | (val & 0x1F);
        }
        else if (mbc_num == 3)
        {   // Set lower 7 bits of ROM bank number
            curr_rom_bank = val & 0x7F;
        }
		else if (mbc_num == 5)
        {
            if (pos < 0x3000)
            {   // Set lower 8 bits of ROM bank number
                curr_rom_bank = (curr_rom_bank & 0x0100) | val;
            }
            else if (pos >= 0x3000 && pos <= 0x3FFF)
            {   // Set 9th bit of ROM bank number
                curr_rom_bank = (curr_rom_bank & 0x00FF) | (static_cast<uint16_t>(val & 0x01) << 8);
            }
        }

		// Don't use ROM banks 0x00, 0x20, 0x40, or 0x60 for MBC1
        if (mbc_num == 1)
        {
            switch (curr_rom_bank)
            {
            case 0x20:
            case 0x040:
            case 0x60:
                curr_rom_bank++;
                break;
            }
        }

        curr_rom_bank %= num_rom_banks;
		break;


		// ROM 0x01 - 0x_F
	case 0x4000:
	case 0x5000:
        // 0x4000 - 0x5FFF : Set RAM bank number or Set upper bits of ROM bank number
        if (mbc_num == 0)
        {
            return;
        }

        if (mbc_num == 1)
        {
            if (rom_banking_mode)
            {   // Set upper bits of ROM bank number
                curr_rom_bank = ((curr_rom_bank & 0x001F) | ((val & 0x03) << 5)) % num_rom_banks;
            }
            else if (ram_banking_mode)
            {   // Set RAM bank number 
                //curr_ram_bank = (val & 0x03) % num_ram_banks;
                curr_ram_bank = val % num_ram_banks;
            }
            else
            {
                logger->warn("How'd you get here? Addr: 0x{0:x}, val: 0x{1:x}",
                    pos,
                    val);
            }

            // Don't use ROM banks 0x00, 0x20, 0x40, or 0x60 for MBC1
            switch (curr_rom_bank)
            {
            case 0x20:
            case 0x040:
            case 0x60:
                curr_rom_bank++;
                break;
            }
        }
        else if (mbc_num == 3)
        {
            if (val <= 0x03)
            {   // Set external RAM bank
                curr_ram_bank = val & 0x03;
            }
            else if (val >= 0x08 && val <= 0x0C)
            {   // Set RTC register
                curr_ram_bank = val;
            }
        }
        else if (mbc_num == 5)
        {
            if (val <= 0x0F)
            {
                curr_ram_bank = val;
            }
        }

        break;

	case 0x6000:
	case 0x7000:

        if (mbc_num == 0)
        {
            return;
        }

		// 0x6000 - 0x7FFF : ROM/RAM mode select
        if (mbc_num == 1)
        {
	        if (val == 0x00)
	        {
		        rom_banking_mode = true;
		        ram_banking_mode = false;
	        }
	        else if (val == 0x01)
	        {
		        rom_banking_mode = false;
		        ram_banking_mode = true;
	        }
        } // end mbc_num == 1
        else if (mbc_num == 3)
        {
            prev_mbc3_latch = curr_mbc3_latch;
            curr_mbc3_latch = val;

            if (prev_mbc3_latch == 0x00 &&
                curr_mbc3_latch == 0x01)
            {
                latchCurrTimeToRTC();
            }
        }
        else
        {
	        logger->warn("MBC::setByte() used address: 0x{0:x} with val: 0x{1:x}", pos, val);
        }

		break;

		// RAM 0x00 - 0x03
	case 0xA000:
	case 0xB000:

        if (mbc_num == 1 && external_ram_enabled)
        {
            if (rom_banking_mode)
            {   // Only RAM bank 0x00 can be used during ROM banking mode
                ramBanks[0][pos - 0xA000] = val;
                wroteToRAMBanks = true;
            }
            else
            {
                ramBanks[curr_ram_bank % num_ram_banks][pos - 0xA000] = val;
                wroteToRAMBanks = true;
            }
        }
        else if (mbc_num == 3)
        {
            if (curr_ram_bank <= 0x03 && external_ram_enabled)
            {
                ramBanks[curr_ram_bank % num_ram_banks][pos - 0xA000] = val;
                wroteToRAMBanks = true;
            }
            else if (curr_ram_bank >= 0x08 && curr_ram_bank <= 0x0C && rtc_timer_enabled)
            {
                rtcRegisters[curr_ram_bank - 0x08] = val;
                wroteToRAMBanks = true;
            }
            else
            {
                logger->warn("Tried to write val: 0x{0:x},\taddr: 0x{1:x},\tRAM bank: 0x{2:x} but writing to external RAM is disabled",
                    val,
                    pos,
                    curr_ram_bank);
            }
        }
        else if (mbc_num != 1)
        {
            if (external_ram_enabled)
            {
                ramBanks[curr_ram_bank % num_ram_banks][pos - 0xA000] = val;
                wroteToRAMBanks = true;
            }
            else
            {
                logger->warn("Tried to write val: 0x{0:x},\taddr: 0x{1:x},\tRAM bank: 0x{2:x} but writing to external RAM is disabled",
                    val,
                    pos,
                    curr_ram_bank);
            }
        }
		break;


	default:
		logger->warn("MBC::setByte() used address: 0x{0:x} with val: 0x{1:x}", pos, val);
	}
}

void MBC::loadSaveIntoRAM(const std::string & filename)
{
    // Open file
    std::ifstream file;
    file.open(filename, std::ios::binary);

    if (file.is_open())
    {   // Read in file
        logger->info("Reading in save file");
        std::vector<unsigned char> ram(
            (std::istreambuf_iterator<char>(file)),
            (std::istreambuf_iterator<char>()));

        logger->info("Finished reading in save file");

        uint8_t use_ram_bank = 0;
        uint32_t num_ram_banks_in_file = ram.size() / 0x1FFF;
        
        // Write file to RAM
        for (uint8_t i = 0; i < num_ram_banks_in_file; i++)
        {   // 0x1FFF = 0xB1FFF - 0xA000
            uint16_t offset = use_ram_bank * 0x2000;

            ramBanks[use_ram_bank] = std::vector<unsigned char>(
                ram.begin() + offset,
                ram.begin() + offset + 0x2000);

            use_ram_bank++;
        }

        // Close file
        file.close();
    }
}

void MBC::saveRAMToFile(const std::string & filename)
{
    if (!wroteToRAMBanks)
    {
        logger->info("Game did not write to RAM, not writing out .sav file");
        return;
    }

    // Open file
    std::ofstream file;
    file.open(filename, std::ios::binary);

    // Write RAM to file
    for (size_t i = 0; i < ramBanks.size(); i++)
    {
        file.write(reinterpret_cast<const char *>(ramBanks[i].data()), ramBanks[i].size());
    }

    // Close file
    file.close();
}

void MBC::latchCurrTimeToRTC()
{
    uint16_t dayCounter;
    uint8_t & seconds   = rtcRegisters[0];
    uint8_t & minutes   = rtcRegisters[1];
    uint8_t & hours     = rtcRegisters[2];
    uint8_t & lower_8_bits_of_day_counter = rtcRegisters[3];
    uint8_t & rtc_DH    = rtcRegisters[4];

    // Get prev day as uint16_t
    uint16_t prevDayCounter = rtc_DH & 0x01;
    prevDayCounter = (prevDayCounter << 8) | lower_8_bits_of_day_counter;

    uint8_t prevHours = hours;

    // Get current time
    const std::time_t currTime = std::time(NULL);

    // Convert to local calendar time
    const std::tm calendarTime = *std::localtime(std::addressof(currTime));

    // Set RTC registers
    seconds = calendarTime.tm_sec;
    minutes = calendarTime.tm_min;
    hours   = calendarTime.tm_hour;

    // Check for hour overflow
    if (prevHours > hours)
    {   // Update dayCounter RTC registers
        lower_8_bits_of_day_counter = prevDayCounter & 0x00FF;
        rtc_DH = (rtc_DH & 0xFE) | ((prevDayCounter & 0x0100) >> 8);
    }

    // Get current day as uint16_t
    dayCounter = rtc_DH & 0x01;
    dayCounter = (dayCounter << 8) | lower_8_bits_of_day_counter;

    // Check for overflow of dayCounter
    if (prevDayCounter > dayCounter)
    {
        rtc_DH |= 0x80; // Set Day Counter Carry bit
    }
}