#include "stdafx.h"
#include "MBC.h"
#include "Debug.h"

MBC::MBC()
{
	rom_banking_mode = true;
	ram_banking_mode = false;
	external_ram_enabled = false;
    rtc_timer_enabled = false;

	curr_rom_bank = 1;
	curr_ram_bank = 1;
    curr_rtc_register = 0;

	num_rom_banks = 2;
	num_ram_banks = 1;

	romBanks.resize(num_rom_banks, std::vector<unsigned char>(ROM_BANK_SIZE, 0));
	ramBanks.resize(num_ram_banks, std::vector<unsigned char>(RAM_BANK_SIZE, 0));

	mbc_num = 0;

	setFromTo(&rom_from_to, 0x0000, 0x7FFF);
	setFromTo(&ram_from_to, 0xA000, 0xBFFF);
}


MBC::~MBC()
{
    logger.reset();
}

void MBC::MBC_init(int mbcNum)
{
	rom_banking_mode = true;
	ram_banking_mode = false;
	external_ram_enabled = false;
	curr_rom_bank = 1;
	curr_ram_bank = 1;

	mbc_num = mbcNum;

	switch (mbc_num)
	{
	case 1: MBC1_init(); break;
	case 2: MBC2_init(); break;
	case 3: MBC3_init(); break;
	case 5: MBC5_init(); break;
	case 6: MBC6_init(); break;
	case 7: MBC7_init(); break;

	default:
		rom_banking_mode = true;
		ram_banking_mode = false;
		external_ram_enabled = false;
		curr_rom_bank = 1;
		curr_ram_bank = 1;

		num_rom_banks = 2;
		num_ram_banks = 1;

		romBanks.resize(num_rom_banks, std::vector<unsigned char>(ROM_BANK_SIZE, 0));
		ramBanks.resize(num_ram_banks, std::vector<unsigned char>(RAM_BANK_SIZE, 0));

		mbc_num = 0;

		setFromTo(&rom_from_to, 0x0000, 0x7FFF);
		setFromTo(&ram_from_to, 0xA000, 0xBFFF);
	}
}

void MBC::MBC1_init()
{
	rom_banking_mode = true;
	ram_banking_mode = true;
	external_ram_enabled = true;
	curr_rom_bank = 1;
	curr_ram_bank = 1;

	num_rom_banks = 0x80;
	num_ram_banks = 0x04;

	romBanks.resize(num_rom_banks, std::vector<unsigned char>(ROM_BANK_SIZE, 0));
	ramBanks.resize(num_ram_banks, std::vector<unsigned char>(RAM_BANK_SIZE, 0));

	setFromTo(&rom_from_to, 0x4000, 0x7FFF);
	setFromTo(&ram_from_to, 0xA000, 0xBFFF);

	mbc_num = 1;
}


/*
	16 ROM banks
	Doesn't support external RAM, includes 512 4-bits of built-in RAM; only the lower 4 bits of the bytes in this memory are used
*/
void MBC::MBC2_init()
{
	rom_banking_mode = true;
	ram_banking_mode = true;
	external_ram_enabled = true;
	curr_rom_bank = 1;
	curr_ram_bank = 1;

	num_rom_banks = 0x10;
	num_ram_banks = 0x04;

	romBanks.resize(num_rom_banks, std::vector<unsigned char>(ROM_BANK_SIZE, 0));
	ramBanks.resize(num_ram_banks, std::vector<unsigned char>(RAM_BANK_SIZE, 0));

	setFromTo(&rom_from_to, 0x4000, 0x7FFF);
	setFromTo(&ram_from_to, 0xA000, 0xA1FF);

	mbc_num = 2;
}


void MBC::MBC3_init()
{
	rom_banking_mode = true;
	ram_banking_mode = true;
	external_ram_enabled = true;
	curr_rom_bank = 1;
	curr_ram_bank = 1;

	num_rom_banks = 0x80;
	num_ram_banks = 0x04;

	romBanks.resize(num_rom_banks, std::vector<unsigned char>(ROM_BANK_SIZE, 0));
	ramBanks.resize(num_ram_banks, std::vector<unsigned char>(RAM_BANK_SIZE, 0));

	setFromTo(&rom_from_to, 0x4000, 0x7FFF);
	setFromTo(&ram_from_to, 0xA000, 0xBFFF);

	mbc_num = 3;
}

void MBC::MBC5_init()
{
    rom_banking_mode = true;
    ram_banking_mode = true;
    external_ram_enabled = true;
    curr_rom_bank = 1;
    curr_ram_bank = 1;

    num_rom_banks = 0x01FF;
    num_ram_banks = 0x0F;

    romBanks.resize(num_rom_banks, std::vector<unsigned char>(ROM_BANK_SIZE, 0));
    ramBanks.resize(num_ram_banks, std::vector<unsigned char>(RAM_BANK_SIZE, 0));

    setFromTo(&rom_from_to, 0x4000, 0x7FFF);
    setFromTo(&ram_from_to, 0xA000, 0xBFFF);

    mbc_num = 5;
}


void MBC::MBC6_init()
{

}


void MBC::MBC7_init()
{

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
		return romBanks[curr_rom_bank % num_rom_banks][pos - 0x4000];

		// External RAM
	case 0xA000:
	case 0xB000:
		//return external_ram[pos - 0xA000];
        return ramBanks[curr_ram_bank % num_ram_banks][pos - 0xA000];

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
        else
        {
            external_ram_enabled = false;
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
        {
            curr_rom_bank = (curr_rom_bank & 0x0060) | (val & 0x1F);	// Set lower 5 bits of ROM bank number
        }
        else if (mbc_num == 2 && (pos & 0x0100) > 0)
        {
            curr_rom_bank = (curr_rom_bank & 0x0060) | (val & 0x1F);	// Set lower 5 bits of ROM bank number
        }
        else if (mbc_num == 3)
		{
			curr_rom_bank = (curr_rom_bank & 0x0060) | (val & 0x1F);	// Set lower 5 bits of ROM bank number

			// Write the RAM bank number to this address (?) http://gbdev.gg8.se/wiki/articles/MBC3
			romBanks[curr_rom_bank % num_rom_banks][pos] = curr_ram_bank;
		}
			
        if (mbc_num == 5)
        {
            if ((pos & 0xF000) < 0x3000)
            {
                curr_rom_bank = (curr_rom_bank & 0x0100) | val;	// Set lower 8 bits of ROM bank number
            }
            else if (pos >= 0x3000 && pos <= 0x3FFF)
            {
                curr_rom_bank = (curr_rom_bank & 0x00FF) | ((val & 0x01) << 8);// Set upper bit of ROM bank number
            }
            else if (pos >= 0x4000 && pos <= 0x5FFF)
            {
                curr_ram_bank = (val & 0x0F);
            }
        }

		// Don't use ROM banks 0x00, 0x20, 0x40, or 0x60 for MBC1
        if (mbc_num == 1 &&
            ((val & 0x1F) == 0x00 ||
            (val & 0x1F) == 0x20 ||
            (val & 0x1F) == 0x40 ||
            (val & 0x1F) == 0x60))
        {
            curr_rom_bank += 1;
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

        if (rom_banking_mode && mbc_num == 1)
        {   // Set upper bits of ROM bank number
            curr_rom_bank = ((curr_rom_bank & 0x001F) | ((val & 0x03) << 5)) % num_rom_banks;
        }
        else if (mbc_num == 1)
        {   // Set RAM bank number 
            /// TODO: MBC3 RTC Register select
            curr_ram_bank = (val & 0x03) % num_ram_banks;
        }
        else if (mbc_num == 5)
        {
            if (val <= 0x07)
            {   // Set external RAM bank
                curr_ram_bank = val % num_ram_banks;
            }
            else if (val >= 0x08 && val <= 0x0C)
            {   // Set RTC register
                curr_rtc_register = val;
            }
        }

	case 0x6000:
	case 0x7000:

        if (mbc_num == 0)
        {
            return;
        }

		// 0x6000 - 0x7FFF : ROM/RAM mode select
        /// TODO: MBC3 Latch Clock Data
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
        else
        {
	        logger->warn("MBC::setByte() used address: 0x{0:x} with val: 0x{1:x}", pos, val);
        }

		break;


		// RAM 0x00 - 0x03
	case 0xA000:
	case 0xB000:

        if (ram_banking_mode)
        {
            ramBanks[curr_ram_bank % num_ram_banks][pos - 0xA000] = val;
        }
		break;


	default:
		logger->warn("MBC::setByte() used address: 0x{0:x} with val: 0x{1:x}", pos, val);
	}
}

