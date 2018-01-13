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

public:

	CartridgeReader();
	CartridgeReader(std::string filename);
	~CartridgeReader();

	void setRomDestination(std::string filename);
	bool readRom();
	void getCartridgeInformation();

	std::int8_t readByte(std::int16_t pos);
	void setByte(std::int16_t pos, int8_t val);
};
#endif