#pragma once

#ifndef MBC_H
#define MBC_H

#include <vector>

class MBC
{

#define ROM_BANK_SIZE 0x4000
#define RAM_BANK_SIZE 0x2000

private:


public:

	MBC();
	MBC(int mbc_num);
	~MBC();

	int mbc_num;
	void MBC1_init();
	void MBC2_init();
	void MBC3_init();
	void MBC5_init();
	void MBC6_init();
	void MBC7_init();

	std::vector<std::vector<unsigned char>> romBanks;	// size per bank = 16 KB = 0x4000
	std::vector<std::vector<unsigned char>> ramBanks;	// size per bank = 8 KB = 0x2000

	std::uint8_t curr_rom_bank;
	std::uint8_t curr_ram_bank;
	int num_rom_banks;
	int num_ram_banks;

	bool rom_mode;	// RAM bank 0 can be used during this mode
	bool ram_mode;
};
#endif