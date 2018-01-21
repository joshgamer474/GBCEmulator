#include "MBC.h"


MBC::MBC()
{


}


MBC::MBC(int mbcNum)
{
	rom_mode = true;
	ram_mode = false;
	curr_rom_bank = 0;
	curr_ram_bank = 0;

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
		MBC(); break;
	}
}

MBC::~MBC()
{

}


void MBC::MBC1_init()
{
	num_rom_banks = 0x80;
	num_ram_banks = 0x04;

	romBanks.resize(num_rom_banks, std::vector<unsigned char>(ROM_BANK_SIZE, 0));
	ramBanks.resize(num_ram_banks, std::vector<unsigned char>(RAM_BANK_SIZE, 0));
}


void MBC::MBC2_init()
{

}


void MBC::MBC3_init()
{

}

void MBC::MBC5_init()
{

}


void MBC::MBC6_init()
{

}


void MBC::MBC7_init()
{

}


