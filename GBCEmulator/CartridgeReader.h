#pragma once

#ifndef CARTRIDGE_READER_H
#define CARTRIDGE_READER_H

#include <string>
#include <fstream>
#include <iterator>
#include <vector>

class CartridgeReader
{

private:

	std::string cartridgeFilename;
	std::vector<char> romBuffer;

	// Information about the cartridge
	char game_title[16];
	char manufacturer_code[4];
	char cgb_flag;
	char sgb_flag;
	char cartridge_type;
	char rom_size;
	char ram_size;
	char destination_code;	// 0x00 = Japanese, 0x01 = non-Japanese
	char game_version;
	char header_checksum;	// Checks if cartridge is valid

	int getNumOfRomBanks(char rom_size);
	int getNumOfRamBanks(char ram_size);

	struct CartridgeType
	{
		int mbc;
		bool rom, ram, battery, timer, rumble, sensor, mmm01;
	};

	void getCartridgeType(char cartridge_type, CartridgeType *cartridgeType);


public:

	CartridgeReader();
	CartridgeReader(std::string filename);
	~CartridgeReader();

	CartridgeType cartridgeType;
	int num_ROM_banks;
	int num_RAM_banks;

	void setRomDestination(std::string filename);
	bool readRom();
	void getCartridgeInformation();

	std::int8_t readByte(std::int16_t pos);
	void setByte(std::int16_t pos, int8_t val);
};
#endif