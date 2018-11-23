#pragma once

#ifndef MBC_H
#define MBC_H

#include <vector>
#include <spdlog/spdlog.h>

class MBC
{
public:
	MBC();
	~MBC();

	void MBC_init(int mbc_num);
	void MBC1_init();
	void MBC2_init();
	void MBC3_init();
	void MBC5_init();
	void MBC6_init();
	void MBC7_init();

	// Reading and writing methods
	uint8_t readByte(std::uint16_t pos);
	void setByte(std::uint16_t pos, std::uint8_t val);

    const uint16_t ROM_BANK_SIZE = 0x4000;
	const uint16_t RAM_BANK_SIZE = 0x2000;

	std::vector<std::vector<unsigned char>> romBanks;	// size per bank = 16 KB = 0x4000
	std::vector<std::vector<unsigned char>> ramBanks;	// size per bank = 8 KB = 0x2000
    std::vector<unsigned char> rtcRegisters;

	uint16_t curr_rom_bank;
	uint8_t curr_ram_bank;
	int mbc_num;
	int num_rom_banks;
	int num_ram_banks;

	bool rom_banking_mode;	// RAM bank 0 can be used during this mode
	bool ram_banking_mode;
    bool rtc_timer_enabled;
	bool external_ram_enabled;

	unsigned char external_ram[0x2000];

	std::shared_ptr<spdlog::logger> logger;

	struct From_To
	{
		int start, end;
	};
	From_To rom_from_to;
	From_To ram_from_to;
	void setFromTo(From_To *, int start, int end);
};
#endif