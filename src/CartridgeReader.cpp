#include "CartridgeReader.h"
#include "Debug.h"
#include "MBC.h"
#include <zip.h>

CartridgeReader::CartridgeReader(std::shared_ptr<spdlog::logger> _logger, const bool _force_cgb)
    : has_bios(false)
    , is_in_bios(false)
    , bios_is_cgb(false)
    , game_title_hash(0)
    , game_title_hash_16(0)
    , force_cgb(_force_cgb)
    , logger(_logger)
{

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
    bios_is_cgb         = rhs.bios_is_cgb;
    romBuffer           = rhs.romBuffer;
    cartridgeFilename   = rhs.cartridgeFilename;
    game_title_str      = rhs.game_title_str;
    game_title_hash     = rhs.game_title_hash;
    game_title_hash_16  = rhs.game_title_hash_16;

    // Information about the cartridge
    memcpy(game_title, rhs.game_title, 16);
    memcpy(manufacturer_code, rhs.manufacturer_code, 4);
    memcpy(new_licensee_code, rhs.new_licensee_code, 2);
    old_licensee_code   = rhs.old_licensee_code;
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

void CartridgeReader::setRomDestination(const std::string& filename)
{
	cartridgeFilename = filename;
}

void CartridgeReader::setBiosDestination(const std::string& bios_filename)
{
    biosFilename = bios_filename;
}

bool CartridgeReader::readBios()
{
    bios = readFile(biosFilename);
    has_bios = is_in_bios = !bios.empty();
    logger->info("Read in BIOS {}, has_bios: {}",
        biosFilename, has_bios);

    // Check if read in BIOS is GB or CGB
    if (bios.size() > 0xFF)
    {
        bios_is_cgb = true;
    }
    return !bios.empty();
}

bool CartridgeReader::readRom()
{
    logger->info("Is file .zip: {}", endsWith(cartridgeFilename, ".zip"));
    if (endsWith(cartridgeFilename, ".zip"))
    {
        logger->info("Attempting to open .zip..");
        const auto compressed_data = romBuffer;
        const int ret = uncompressZip(cartridgeFilename, romBuffer);
    }
    else
    {
        romBuffer = readFile(cartridgeFilename);
    }

    if (romBuffer.size())
    {
        // Read information from cartridge
        getCartridgeInformation();
        logger->info("Finished reading in {}, file size: {}",
            game_title,
            romBuffer.size());
        return true;
    }
    return false;
}

std::vector<unsigned char> CartridgeReader::readFile(const std::string& filename) const
{
    std::ifstream in;
    std::vector<unsigned char> out;

    logger->info("Reading in file {}", filename);
    in.open(filename, std::ios::binary);
    in.unsetf(std::ios::skipws);   // Don't skip newlines when reading as binary

    if (in.is_open())
    {
        // Get size of ROM
        std::streampos fileSize;
        in.seekg(0, std::ios::end);
        fileSize = in.tellg();

        logger->info("File size is {0:d} bytes", fileSize);

        // Seek back to the beginning of the ROM
        in.seekg(0, std::ios::beg);

        // Reserve space in romBuffer
        out.reserve(fileSize);

        // Read ROM into vector
        out.insert(out.begin(),
            std::istream_iterator<unsigned char>(in),
            std::istream_iterator<unsigned char>());

        logger->info("Finished reading in {}", filename);

        in.close();
    }
    else
    {
        logger->critical("Could not read in file {}", filename);
    }

    return out;
}

int CartridgeReader::uncompressZip(const std::string& filename, std::vector<unsigned char>& uncompressed_out) const
{
    logger->info("Attempting to open .zip {}", filename);

    int ret = 0;
    zip_t* zip = zip_open(filename.c_str(), ZIP_RDONLY, &ret);

    if (zip == NULL)
    {
        logger->error("Failed to open .zip {}", filename);
        return -1;
    }

    logger->info("Number of entries in .zip: {}, parsing entries..",
        zip_get_num_entries(zip, ZIP_RDONLY));

    // Find ROM in .zip
    struct zip_stat st;

    for (int i = 0; i < zip_get_num_entries(zip, ZIP_RDONLY); i++)
    {
        if (zip_stat_index(zip, i, ZIP_RDONLY, &st) == 0)
        {
            logger->info("Entry {}, name: {}, size: {}, valid: {}, mtime: {}",
                i,
                st.name,
                st.size,
                st.valid,
                st.mtime);

            // Check if internal file is .gb or .gbc
            if (!romIsValid(st.name))
            {
                logger->info("Entry {} is not a valid .gb or .gbc ROM", i);
                continue;
            }

            // Read in zipped file
            zip_file* zf = zip_fopen_index(zip, i, ZIP_RDONLY);
            if (!zf)
            {   // Failed ot open entry in zip, close zip
                zip_close(zip);
            }

            uncompressed_out.resize(st.size);
            const int len = zip_fread(zf, uncompressed_out.data(), st.size);
            zip_fclose(zf);

            // Done reading entry, close opened zip entry
            logger->info("Done reading .zip entry {}, read {} bytes, uncompressed_out.size(): {}",
                st.name,
                len,
                uncompressed_out.size());
            break;
        }
    }

    return ret;
}

// Information about the cartridge
void CartridgeReader::getCartridgeInformation()
{
    // Read game title
    game_title_hash = 0;
    game_title_hash_16 = 0;
    for (int i = 0; i < 16; i++)
    {
        const uint8_t& c = romBuffer[i + 0x0134];
        if (c == 0x00)
        {
            break;
        }
        game_title[i]    = c;
        game_title_hash += c;
        game_title_hash_16 += c;
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
    old_licensee_code   = romBuffer[0x014B];
    if (old_licensee_code == 0x33)
    {
        new_licensee_code[0] = romBuffer[0x0144];
        new_licensee_code[0] = romBuffer[0x0145];
    }
	game_version        = romBuffer[0x014C];
 	header_checksum     = romBuffer[0x014D];

    logger->info("Parsed cartridge, game_title: {}, game_title_hash: {}, game_title_hash_16: {}, "
        "old_licensee_code: {}, header_checksum: {}",
        game_title,
        game_title_hash,
        game_title_hash_16,
        old_licensee_code,
        header_checksum);
}

bool CartridgeReader::getColorGBFlag() const
{
	return cgb_flag;
}

bool CartridgeReader::isColorGB() const
{
    if (has_bios)
    {
        return bios_is_cgb
            || (cgb_flag & 0x80)
            || force_cgb;
    }
    return (cgb_flag & 0x80) || force_cgb;
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

bool CartridgeReader::endsWith(const std::string& str, const std::string& suffix) const
{
    return str.size() >= suffix.size() &&
        0 == str.compare(str.size() - suffix.size(),
                suffix.size(),
                suffix);
}

std::string CartridgeReader::getFileExtension(const std::string& filepath) const
{
    std::string ret = "";
    size_t index = filepath.find_last_of(".");
    if (index != std::string::npos)
    {   // Found at least one "."
        ret = filepath.substr(index + 1);
    }

    return ret;
}

bool CartridgeReader::romIsValid(const std::string& filepath) const
{
    bool ret = false;
    std::string fileExtension = getFileExtension(filepath);

    // Make file extension lowercase
    std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);

    if (fileExtension.rfind(".gb") ||
        fileExtension.rfind(".gbc"))
    {
        ret = true;
    }

    return ret;
}