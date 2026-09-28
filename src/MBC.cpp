#ifdef _WIN32
#include "stdafx.h"
#endif // _WIN32

#include "MBC.h"
#include "Debug.h"
#include <fstream>
#include <chrono>
#include <ctime>
#include <exception>

int64_t rtc_unix_seconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

MBC::MBC(int mbcNum, int numROMBanks, int numRAMBanks, std::shared_ptr<spdlog::logger> _logger)
{
    rom_banking_mode = true;
    ram_banking_mode = false;
    external_ram_enabled = false;
    wroteToRAMBanks  = false;
    wroteToRTC       = false;
    auto_save        = false;
    curr_rom_bank = 1;
    curr_ram_bank = 1;

    num_rom_banks = numROMBanks;
    num_ram_banks = numRAMBanks;

    if (num_ram_banks == 0)
    {
        num_ram_banks++;
    }

    romBanks.resize(num_rom_banks, std::vector<unsigned char>(ROM_BANK_SIZE, 0));
    ramBanks.resize(num_ram_banks, std::vector<unsigned char>(RAM_BANK_SIZE, 0));

    mbc_num = mbcNum;
    mbc_type = static_cast<MBC_Type>(mbcNum);

    rtc_last_update = std::chrono::steady_clock::now();
    rtc_timer_enabled = false;
    prev_mbc3_latch = 0xFF;
    curr_mbc3_latch = 0xFF;

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
    if (logger)
    {
        logger.reset();
    }
}

MBC& MBC::operator=(const MBC& rhs)
{   // Copy from rhs
    rom_banking_mode        = rhs.rom_banking_mode;
    ram_banking_mode        = rhs.ram_banking_mode;
    external_ram_enabled    = rhs.external_ram_enabled;
    rtc_timer_enabled       = rhs.rtc_timer_enabled;
    latched_rtc_registers   = rhs.latched_rtc_registers;
    rtc_last_update         = rhs.rtc_last_update;

    curr_rom_bank   = rhs.curr_rom_bank;
    curr_ram_bank   = rhs.curr_ram_bank;
    num_rom_banks   = rhs.num_rom_banks;
    num_ram_banks   = rhs.num_ram_banks;
    romBanks        = rhs.romBanks;
    ramBanks        = rhs.ramBanks;
    rtcRegisters    = rhs.rtcRegisters;
    mbc_num         = rhs.mbc_num;
    mbc_type        = rhs.mbc_type;

    rom_from_to = rhs.rom_from_to;
    ram_from_to = rhs.ram_from_to;

    prev_mbc3_latch = rhs.prev_mbc3_latch;
    curr_mbc3_latch = rhs.curr_mbc3_latch;
    wroteToRAMBanks = rhs.wroteToRAMBanks;
    wroteToRTC      = rhs.wroteToRTC;

    return *this;
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
    rtcRegisters.fill(0);
    latched_rtc_registers.fill(0);
    rtc_last_update = std::chrono::steady_clock::now();

    rtc_timer_enabled = false;
    prev_mbc3_latch = 0xFF;
    curr_mbc3_latch = 0xFF;

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


std::uint8_t MBC::readByte(const uint16_t pos) const
{
	switch (pos & 0xF000)
	{
		// ROM 00
	case 0x0000:
	case 0x1000:
	case 0x2000:
	case 0x3000:
        logger->trace("Reading from ROM bank: 0");
		return romBanks[0][pos];

		// ROM 01 - N
	case 0x4000:
	case 0x5000:
	case 0x6000:
	case 0x7000:
        if (mbc_num == 1 && ram_banking_mode)
        {   // Only ROM banks 0x00-0x1F can be used during RAM banking mode
            logger->trace("Reading from ROM bank: {}", curr_rom_bank % 0x1F);
            return romBanks[curr_rom_bank % 0x1F][pos - 0x4000];
        }
        /*if (mbc_num == 5 && ram_banking_mode)
        {    // Only ROM banks 0x00-0x01FF can be used during RAM banking mode
            logger->trace("Reading from ROM bank: {}", curr_rom_bank % 0x1FF);
            return romBanks[curr_rom_bank % 0x1FF][pos - 0x4000];
        }*/
        
        // All ROM banks can be accessed
        logger->trace("Reading from ROM bank: {}", curr_rom_bank % num_rom_banks);
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
                logger->trace("Reading from RAM bank: 0");
                return ramBanks[0][pos - 0xA000];
            }
            else if (ram_banking_mode)
            {   // ram_banking_mode == true
                logger->trace("Reading from RAM bank: {}", curr_ram_bank % num_ram_banks);
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
                logger->trace("Reading from RAM bank: {}", curr_ram_bank % num_ram_banks);
                return ramBanks[curr_ram_bank % num_ram_banks][pos - 0xA000];
            }
            else if (curr_ram_bank >= 0x08 && curr_ram_bank <= 0x0C)
            {
                logger->trace("Reading from RTC register: {}", curr_ram_bank - 0x08);
                //return rtcRegisters[curr_ram_bank - 0x08];
                return latched_rtc_registers[curr_ram_bank - 0x08];
            }
        }
        else
        {
            logger->trace("Reading from RAM bank: {}", curr_ram_bank % num_ram_banks);
            return ramBanks[curr_ram_bank % num_ram_banks][pos - 0xA000];
        }

	default:
		logger->warn("MBC::readByte() used address: 0x{0:x}", pos);
		return 0;
	}
}


void MBC::setByte(const uint16_t pos, uint8_t val)
{
    if (auto_save)
    {
        if (wroteToRAMBanks &&
            (pos < 0xA000 ||
            pos > 0xBFFF))
        {   // Done writing to external RAM banks,
            // auto-write out to .sav B)
            logger->info("Auto saving external RAM to {}",
                savFilename);
            saveRAMToFile(savFilename);
            wroteToRAMBanks = false;
        }

        if (wroteToRTC &&
            (pos < 0xA000 ||
            pos > 0xBFFF))
        {   // Done writing to external RAM banks,
            // auto-write out to .sav B)
            logger->info("Auto saving RTC bytes to {}",
                rtcFilename);
            saveRAMToFile(rtcFilename);
            wroteToRTC = false;
        }
    }

	switch (pos & 0xF000)
	{
		// ROM 00
	case 0x0000:
	case 0x1000:

        // 0x0000 - 0x1FFF : Set RAM enable
        external_ram_enabled = (val & 0x0F) == 0x0A;
        if (external_ram_enabled)
        {
            if (mbc_num == 3)
            {
                rtc_timer_enabled = true;
            }
        }
        else
        {
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
            case 0x40:
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
            /*if (rom_banking_mode)
            {   // Set upper bits of ROM bank number
                curr_rom_bank = ((curr_rom_bank & 0x001FF) | ((val & 0x0F) << 5)) % num_rom_banks;
            }
            else if (ram_banking_mode && val <= 0x0F)
            {   // Set RAM bank number 
                //curr_ram_bank = (val & 0x03) % num_ram_banks;
                curr_ram_bank = val % num_ram_banks;
            }
            else
            {
                logger->warn("How'd you get here? Addr: 0x{0:x}, val: 0x{1:x}",
                    pos,
                    val);
            }*/

            // Don't use ROM banks 0x00, 0x20, 0x40, or 0x60 for MBC1
            /*switch (curr_rom_bank)
            {
            case 0x20:
            case 0x40:
            case 0x60:
                curr_rom_bank++;
                break;
            }*/
            curr_ram_bank = val & 0x0F;
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
                advance_rtc();

                const bool wasHalted = rtcRegisters[4] & 0x40;
                switch (curr_ram_bank - 0x08)
                {
                    case 0: rtcRegisters[0] = val & 0x3F; break;
                    case 1: rtcRegisters[1] = val & 0x3F; break;
                    case 2: rtcRegisters[2] = val & 0x1F; break;
                    case 3: rtcRegisters[3] = val; break;
                    case 4: rtcRegisters[4] = val & 0xC1; break; // Bit 8, halt, and carry are writable
                }
                //rtcRegisters[curr_ram_bank - 0x08] = val;

                const bool isHalted = rtcRegisters[4] & 0x40;
                if (wasHalted != isHalted || (curr_ram_bank - 0x08) == 0)
                {
                    rtc_last_update = std::chrono::steady_clock::now();
                }

                wroteToRTC = true;
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
                /*if (rom_banking_mode)
                {   // Only RAM bank 0x00 can be used during ROM banking mode
                    ramBanks[0][pos - 0xA000] = val;
                    wroteToRAMBanks = true;
                }
                else*/
                {
                    ramBanks[curr_ram_bank % num_ram_banks][pos - 0xA000] = val;
                    wroteToRAMBanks = true;
                }
                //ramBanks[curr_ram_bank % num_ram_banks][pos - 0xA000] = val;
                //wroteToRAMBanks = true;
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
    savFilename = filename;

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

void MBC::loadRTCIntoRAM(const std::string & filename)
{
    if (mbc_num != 3)
    {
        logger->info("MBC{} does not support RTC clock",
            mbc_num);
        return;
    }

    rtcFilename = filename;
    rtc_last_update = std::chrono::steady_clock::now();

    // Open file
    std::ifstream file;
    file.open(filename, std::ios::binary);
    if (!file)
    {
        return;
    }

    // Read in file
    logger->info("Reading in RTC file");
    std::vector<unsigned char> data(
        (std::istreambuf_iterator<char>(file)),
        (std::istreambuf_iterator<char>()));

    // Verify format is correct
    uint64_t saved_timestamp = 0;
    bool found_timestamp = false;
    if (data.size() == 17 &&
        data[0] == 'R' &&
        data[1] == 'T' &&
        data[2] == 'C' &&
        data[3] == '1')
    {
        // Read in saved RTC registers
        uint8_t offset = 4;
        for (uint8_t i = 0; i < 5; i++)
        {
            rtcRegisters[i] = data[i + offset];
        }
        offset += 5;

        // Read in saved timestamp
        for (uint8_t i = 0; i < 8; i++)
        {
            saved_timestamp |= static_cast<uint64_t>(data[i + offset]) << (i * 8);
        }
        found_timestamp = true;
    }
    else
    {
        logger->error("Failed to decode RTC file {}", filename);
        return;
    }

    // Ensure RTC register bits
    rtcRegisters[0] &= 0x3F;
    rtcRegisters[1] &= 0x3F;
    rtcRegisters[2] &= 0x1F;
    rtcRegisters[3] &= 0xFF;
    rtcRegisters[4] &= 0xC1;

    const int64_t now = rtc_unix_seconds();
    if (found_timestamp &&
        now >= 0 &&
        static_cast<uint64_t>(now) > saved_timestamp)
    {
        add_rtc_seconds(static_cast<uint64_t>(now) - saved_timestamp);
    }
    rtc_last_update = std::chrono::steady_clock::now();

    // Update RTC registers to internal RTC clock
    latched_rtc_registers = rtcRegisters;

    wroteToRTC = false;

    // Close file
    file.close();
}

void MBC::saveRAMToFile(const std::string & filename)
{
    if (!wroteToRAMBanks || ramBanksAreEmpty())
    {
        logger->info("Game did not write to RAM, not writing out .sav file");
        return;
    }

    // Open file
    std::ofstream file;
    file.open(filename, std::ios::binary);

    logger->info("Saving RAM to {}", filename);

    // Write RAM to file
    for (size_t i = 0; i < ramBanks.size(); i++)
    {
        file.write(reinterpret_cast<const char *>(ramBanks[i].data()), ramBanks[i].size());
    }

    // Close file
    file.close();
}

void MBC::saveRTCToFile(const std::string & filename)
{
    if (mbc_num != 3)
    {
        logger->info("MBC{} does not have RTC registers to write out",
            mbc_num);
        return;
    }

    // Increment RTC clock
    advance_rtc();

    // Get current time in seconds
    const int64_t now = rtc_unix_seconds();

    // Open file
    std::ofstream file;
    file.open(filename, std::ios::binary | std::ios::trunc);
    if (!file)
    {
        logger->error("Could not open RTC file {}", filename);
        return;
    }

    logger->info("Saving RTC to {}", filename);

    // Write RTC to file
    //file.write(reinterpret_cast<const char *>(rtcRegisters.data()), rtcRegisters.size());
    file.write("RTC1", 4);
    file.write(reinterpret_cast<const char*>(rtcRegisters.data()), rtcRegisters.size());

    const uint64_t timestamp = static_cast<uint64_t>(now);
    std::array<uint8_t, 8> atimestamp = {};
    for (uint8_t i = 0; i < 8; i++)
    {
        atimestamp[i] = static_cast<char>((timestamp >> (i * 8)) & 0xFF);
    }
    file.write(reinterpret_cast<const char*>(atimestamp.data()), atimestamp.size());
    file.flush();

    // Close file
    file.close();

    wroteToRTC = false;
}


bool MBC::ramBanksAreEmpty() const
{
    for (size_t i = 0; i < ramBanks.size(); i++)
    {
        for (size_t j = 0; j < ramBanks[i].size(); j++)
        {
            if (ramBanks[i][j])
            {
                return false;
            }
        }
    }
    return true;
}

void MBC::latchCurrTimeToRTC()
{
    if (mbc_num != 3)
    {
        return;
    }

    // Increment RTC registers
    advance_rtc();

    // Copy RTC registers to internal RTC time
    latched_rtc_registers = rtcRegisters;
}

void MBC::add_rtc_seconds(const uint64_t& elapsed)
{
    if (rtcRegisters[4] & 0x40 || elapsed == 0)
    {
        return;
    }

    uint8_t& seconds   = rtcRegisters[0];
    uint8_t& minutes   = rtcRegisters[1];
    uint8_t& hours     = rtcRegisters[2];
    uint8_t& lower_8_bits_of_day_counter = rtcRegisters[3];
    uint8_t& rtc_DH    = rtcRegisters[4];

    // Calculate number of RTC days
    const uint16_t days = lower_8_bits_of_day_counter | ((static_cast<uint16_t>(rtc_DH) & 0x01) << 8);

    const uint64_t secondsPerHour = 60 * 60;
    const uint64_t secondsPerDay = secondsPerHour * 24;
    uint64_t elapsedDays = elapsed / secondsPerDay;
    uint64_t timeOfDay =
        static_cast<uint64_t>(hours) * secondsPerHour +
        static_cast<uint64_t>(minutes) * 60 +
        seconds +
        elapsed % secondsPerDay;

    elapsedDays += timeOfDay / secondsPerDay;
    timeOfDay %= secondsPerDay;

    const uint64_t totalDays = days + elapsedDays;
    const uint64_t wrappedDays = static_cast<uint16_t>(totalDays & 0x1FF);

    uint8_t flags = rtc_DH & 0xC0;
    if (totalDays >= 512)
    {
        flags |= 0x80;
    }

    seconds = static_cast<uint8_t>(timeOfDay % 60);
    minutes = static_cast<uint8_t>((timeOfDay / 60) % 60);
    hours = static_cast<uint8_t>((timeOfDay / secondsPerHour));
    lower_8_bits_of_day_counter = static_cast<uint8_t>(wrappedDays);
    rtc_DH = flags | ((wrappedDays >> 8) & 0x01);
}

void MBC::advance_rtc()
{
    if (mbc_num != 3)
    {
        return;
    }

    const auto now = std::chrono::steady_clock::now();

    // Check if currently halted
    if (rtcRegisters[4] & 0x40)
    {   // Don't increment RTC time when halted
        rtc_last_update = now;
        return;
    }

    // Calculate last time since now
    const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - rtc_last_update);
    if (elapsed.count() > 0)
    {
        add_rtc_seconds(static_cast<uint64_t>(elapsed.count()));
        rtc_last_update += elapsed;
    }
}