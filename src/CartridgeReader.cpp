#include "CartridgeReader.h"
#include "Debug.h"
#include "MBC.h"

CartridgeReader::CartridgeReader(std::shared_ptr<spdlog::logger> _logger)
    : has_bios(false)
    , logger(_logger)
{
    is_in_bios = has_bios;
}

CartridgeReader::~CartridgeReader()
{
    logger.reset();
}

CartridgeReader& CartridgeReader::operator=(const CartridgeReader& rhs)
{   // Copy from rhs
    num_ROM_banks       = rhs.num_ROM_banks;
    num_RAM_banks       = rhs.num_RAM_banks;
    is_in_bios          = rhs.is_in_bios;
    has_bios            = rhs.has_bios;
    romBuffer           = rhs.romBuffer;
    cartridgeFilename   = rhs.cartridgeFilename;
    game_title_str      = rhs.game_title_str;

    // Information about the cartridge
    memcpy(game_title, rhs.game_title, 16);
    memcpy(manufacturer_code, rhs.manufacturer_code, 4);
    cgb_flag            = rhs.cgb_flag;
    sgb_flag            = rhs.sgb_flag;
    cartridge_type      = rhs.cartridge_type;
    rom_size            = rhs.rom_size;
    ram_size            = rhs.ram_size;
    destination_code    = rhs.destination_code;
    game_version        = rhs.game_version;
    header_checksum     = rhs.header_checksum;

    return *this;
}

void CartridgeReader::setRomDestination(std::string filename)
{
	cartridgeFilename = filename;

}

bool CartridgeReader::readRom()
{
	std::ifstream rom;
	rom.open(cartridgeFilename, std::ios::binary);

    rom.unsetf(std::ios::skipws);   // Don't skip newlines when reading as binary

    if (rom.is_open())
    {
        logger->info("Reading in ROM");

        // Get size of ROM
        std::streampos fileSize;
        rom.seekg(0, std::ios::end);
        fileSize = rom.tellg();

        logger->info("ROM size is {0:d} bytes", fileSize);

        // Seek back to the beginning of the ROM
        rom.seekg(0, std::ios::beg);

        // Reserve space in romBuffer
        romBuffer.reserve(fileSize);

        // Read ROM into vector
        romBuffer.insert(romBuffer.begin(),
            std::istream_iterator<unsigned char>(rom),
            std::istream_iterator<unsigned char>());

        // Read information from cartridge
        getCartridgeInformation();
        logger->info("Finished reading in {}", game_title);

        rom.close();
        return true;
	}
	else
	{
        logger->critical("Could not read rom {}", cartridgeFilename);
		return false;
	}
}


// Information about the cartridge
void CartridgeReader::getCartridgeInformation()
{
	// Read game title
    for (int i = 0; i < 16; i++)
    {
        game_title[i] = romBuffer[i + 0x0134];
    }
    game_title[15] = '\0';
    game_title_str = std::string(reinterpret_cast<char*>(game_title));

	// Read in Manufacturer code
    for (int i = 0; i < 4; i++)
    {
        manufacturer_code[i] = romBuffer[i + 0x013F];
    }

	cgb_flag = romBuffer[0x0143];
	sgb_flag = romBuffer[0x0146];

	cartridge_type  = romBuffer[0x0147];
	rom_size        = romBuffer[0x0148];
	ram_size        = romBuffer[0x0149];

	parseCartridgeType(cartridge_type);

	num_ROM_banks = getNumOfRomBanks(rom_size);
    if (num_ROM_banks == 1)
    {
        num_ROM_banks++;
    }
	num_RAM_banks = getNumOfRamBanks(ram_size);
	
	destination_code    = romBuffer[0x014A];
	game_version        = romBuffer[0x014C];
 	header_checksum     = romBuffer[0x014D];
}

bool CartridgeReader::getColorGBFlag() const
{
	return cgb_flag;
}

bool CartridgeReader::isColorGB() const
{
    return cgb_flag & 0x80;
}


int CartridgeReader::getNumOfRomBanks(unsigned char rom_size)
{
	switch (rom_size)
	{
	case 0x00: return 1;
	case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07: case 0x08:
		return std::pow(2, (int)(rom_size + 1));

	case 0x52: return 72;
	case 0x53: return 80;
	case 0x54: return 96;

	default:
        logger->warn("Unknown ROM size byte received: 0x{0:x}", rom_size);
        return 1;
	}
}

int CartridgeReader::getNumOfRamBanks(unsigned char ram_size)
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

void CartridgeReader::parseCartridgeType(unsigned char cartridge_type)
{
    setMBC(cartridge_type);
    setROMSize(cartridge_type);
    setRAMSize(cartridge_type);
    setBattery(cartridge_type);
    setTimer(cartridge_type);
    setMMM01(cartridge_type);
    setRumble(cartridge_type);

	// Sensor
    cartridgeType.sensor = (cartridge_type == 0x22);
}

void CartridgeReader::setMBC(unsigned char cartridge_type)
{
    switch (cartridge_type)
    {
    case 0x01:
    case 0x02:
    case 0x03:
        cartridgeType.mbc = MBC1; break;

    case 0x05:
    case 0x06:
        cartridgeType.mbc = MBC2; break;

    case 0x0F:
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
        cartridgeType.mbc = MBC3; break;

    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1E:
        cartridgeType.mbc = MBC5; break;

    case 0x20: cartridgeType.mbc = MBC6; break;
    case 0x22: cartridgeType.mbc = MBC7; break;
    case 0xFD: cartridgeType.mbc = TAMA5; break;
    case 0xFE: cartridgeType.mbc = HuC3; break;
    case 0xFF: cartridgeType.mbc = HuC1; break;

    default:
        cartridgeType.mbc = 0;
    }
}

void CartridgeReader::setROMSize(unsigned char cartridge_type)
{
    switch (cartridge_type)
    {
    case 0x00:
    case 0x08:
    case 0x09:
        cartridgeType.rom = true; break;

    default:
        cartridgeType.rom = false;
    }
}

void CartridgeReader::setRAMSize(unsigned char cartridge_type)
{
    switch (cartridge_type)
    {
    case 0x02:
    case 0x03:
    case 0x08:
    case 0x09:
    case 0x0C:
    case 0x0D:
    case 0x10:
    case 0x12:
    case 0x13:
    case 0x1A:
    case 0x1B:
    case 0x1D:
    case 0x1E:
    case 0x22:
    case 0xFF:
        cartridgeType.ram = true; break;

    default:
        cartridgeType.ram = false;
    }
}

void CartridgeReader::setBattery(unsigned char cartridge_type)
{
    switch (cartridge_type)
    {
    case 0x03:
    case 0x06:
    case 0x09:
    case 0x0D:
    case 0x0F:
    case 0x10:
    case 0x13:
    case 0x1B:
    case 0x1E:
    case 0x22:
    case 0xFF:
        cartridgeType.battery = true; break;

    default:
        cartridgeType.battery = false;
    }
}

void CartridgeReader::setTimer(unsigned char cartridge_type)
{
    switch (cartridge_type)
    {
    case 0x0F:
    case 0x10:
        cartridgeType.timer = true; break;

    default:
        cartridgeType.timer = false;
    }
}

void CartridgeReader::setMMM01(unsigned char cartridge_type)
{
    switch (cartridge_type)
    {
    case 0x0B:
    case 0x0C:
    case 0x0D:
        cartridgeType.mmm01 = true; break;

    default:
        cartridgeType.mmm01 = false;
        break;
    }
}

void CartridgeReader::setRumble(unsigned char cartridge_type)
{
    switch (cartridge_type)
    {
    case 0x1C:
    case 0x1D:
    case 0x1E:
    case 0x22:
        cartridgeType.rumble = true; break;

    default:
        cartridgeType.rumble = false;
    }
}

int CartridgeReader::getMBCNum() const
{
    return cartridgeType.mbc;
}

void CartridgeReader::freeRom()
{
    romBuffer.resize(0);
    romBuffer.shrink_to_fit();
}

std::string CartridgeReader::getGameTitle() const
{
    return game_title_str;
}