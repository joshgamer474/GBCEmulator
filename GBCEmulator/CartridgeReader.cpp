#include "stdafx.h"
#include "CartridgeReader.h"

CartridgeReader::CartridgeReader()
{
	is_bios = true;
}


CartridgeReader::CartridgeReader(std::string filename)
{
	is_bios = true;

	// Set game cartridge name
	setRomDestination(filename);
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
		std::vector<unsigned char> romBufferr((std::istreambuf_iterator<char>(rom)), (std::istreambuf_iterator<char>()));
		romBuffer = romBufferr;

		// Read information from cartridge
		getCartridgeInformation();
		
		rom.close();
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

	getCartridgeType(cartridge_type, &cartridgeType);
	num_ROM_banks = getNumOfRomBanks(rom_size);
	if (num_ROM_banks == 1) num_ROM_banks++;
	num_RAM_banks = getNumOfRamBanks(ram_size);
	
	destination_code = romBuffer[0x014A];
	game_version = romBuffer[0x014C];
 	header_checksum = romBuffer[0x014D];
}

bool CartridgeReader::getColorGBFlag()
{
	return cgb_flag;
}

std::uint8_t CartridgeReader::readByte(std::uint16_t pos)
{
	printf("Reading byte from pos %#04x\n", pos);
	return romBuffer[pos];
}

void CartridgeReader::setByte(std::uint16_t pos, uint8_t val)
{
	printf("Writing byte %#02x to pos %#04x\n", val, pos);
	romBuffer[pos] = val;
}


int CartridgeReader::getNumOfRomBanks(char rom_size)
{
	switch (rom_size)
	{
	case 0x00: return 1;
	case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07: case 0x08:
		return std::pow(2, (int)(rom_size + 1));

	case 0x52: return 72;
	case 0x53: return 80;
	case 0x54: return 96;

	default: return 0;
	}
}

int CartridgeReader::getNumOfRamBanks(char ram_size)
{
	switch (ram_size)
	{
	case 0x00: return 0;
	case 0x01: case 0x02: return 1;
	case 0x03: return 4;
	case 0x04: return 16;
	case 0x05: return 8;

	default: return 0;
	}
}

void CartridgeReader::getCartridgeType(char cartridge_type, CartridgeType *cartridgeType)
{
	// MBC
	switch (cartridge_type)
	{
	case 0x01: case 0x02: case 0x03:
		cartridgeType->mbc = 1;
		break;

	case 0x05: case 0x06:
		cartridgeType->mbc = 2;
		break;

	case 0x0F: case 0x10: case 0x11: case 0x12: case 0x13:
		cartridgeType->mbc = 3;
		break;

	case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1E:
		cartridgeType->mbc = 5;
		break;

	case 0x20:
		cartridgeType->mbc = 6;
		break;

	case 0x22:
		cartridgeType->mbc = 7;
		break;

	default:
		cartridgeType->mbc = 0;
	}

	// ROM
	switch (cartridge_type)
	{
	case 0x00: case 0x08: case 0x09:
		cartridgeType->rom = true;
		break;

	default:
		cartridgeType->rom = false;
	}

	// RAM
	switch (cartridge_type)
	{
	case 0x02: case 0x03: case 0x08: case 0x09: case 0x0C: case 0x0D:
	case 0x10: case 0x12: case 0x13: case 0x1A: case 0x1B: case 0x1D:
	case 0x1E: case 0x22: case 0xFF:
		cartridgeType->ram = true;
		break;

	default:
		cartridgeType->ram = false;
	}

	// Battery
	switch (cartridge_type)
	{
	case 0x03: case 0x06: case 0x09: case 0x0D: case 0x0F: case 0x10:
	case 0x13: case 0x1B: case 0x1E: case 0x22: case 0xFF:
		cartridgeType->battery = true;
		break;

	default:
		cartridgeType->battery = false;
	}

	// Timer
	switch (cartridge_type)
	{
	case 0x0F: case 0x10:
		cartridgeType->timer = true;
		break;

	default:
		cartridgeType->timer = false;
	}


	// MMM01 (?)
	switch (cartridge_type)
	{
	case 0x0B: case 0x0C: case 0x0D:
		cartridgeType->mmm01 = true;
		break;

	default:
		cartridgeType->mmm01 = false;
		break;
	}

	// Rumble
	switch (cartridge_type)
	{
	case 0x1C: case 0x1D: case 0x1E: case 0x22:
		cartridgeType->rumble = true;
		break;

	default:
		cartridgeType->rumble = false;
	}

	// Sensor
	if (cartridge_type == 0x22)
		cartridgeType->sensor = true;
	else
		cartridgeType->sensor = false;
}


