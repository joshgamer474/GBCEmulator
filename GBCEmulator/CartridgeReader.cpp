#include "stdafx.h"
#include "CartridgeReader.h"

CartridgeReader::CartridgeReader()
{

}


CartridgeReader::CartridgeReader(std::string filename)
{
	// Set game cartridge name
	setRomDestination(filename);

	// Read game cartridge into memory
	if (!readRom())
	{
		printf("Error - Failed to read in cartridge %s", filename.c_str());
	}

	// Read information from cartridge
	getCartridgeInformation();
	
}


CartridgeReader::~CartridgeReader()
{

}



void CartridgeReader::setRomDestination(std::string filename)
{
	cartridgeFilename = filename;

}

bool CartridgeReader::readRom()
{
	std::ifstream rom;
	rom.open(cartridgeFilename, std::ios::binary);

	if (rom.is_open())
	{
		std::vector<char> romBufferr((std::istreambuf_iterator<char>(rom)), (std::istreambuf_iterator<char>()));
		romBuffer = romBufferr;
		return true;
	}
	else
	{
		return false;
	}
}


// Information about the cartridge
void CartridgeReader::getCartridgeInformation()
{
	int i = 0;

	// Read game title
	for (i = 0; i < 16; i++)
		game_title[i] = romBuffer[i + 0x0134];


	// Read in Manufacturer code
	for (i = 0; i < 4; i++)
		manufacturer_code[i] = romBuffer[i + 0x013F];


	cgb_flag = romBuffer[0x0143];
	sgb_flag = romBuffer[0x0146];

	cartridge_type = romBuffer[0x0147];
	rom_size  = romBuffer[0x0148];
	ram_size = romBuffer[0x0149];
	
	destination_code = romBuffer[0x014A];
	game_version = romBuffer[0x014C];
 	header_checksum = romBuffer[0x014D];
}



std::int8_t CartridgeReader::readByte(std::int16_t pos)
{
	printf("Reading byte from pos %#010x\n", pos);
	return romBuffer[pos];
}

void CartridgeReader::setByte(std::int16_t pos, int8_t val)
{
	printf("Writing byte %#010x to pos %#010x\n", val, pos);
	romBuffer[pos] = val;
}



