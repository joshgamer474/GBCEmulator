#ifndef CARTRIDGE_READER_H
#define CARTRIDGE_READER_H

#include <string>
#include <fstream>
#include <iterator>
#include <vector>
#include <spdlog/spdlog.h>

class CartridgeReader
{
public:
	CartridgeReader(std::shared_ptr<spdlog::logger> logger, const bool force_cgb);
	virtual ~CartridgeReader();
    CartridgeReader& operator=(const CartridgeReader& rhs);

	void setRomDestination(const std::string& filename);
    void setBiosDestination(const std::string& bios_filename);
    bool readBios();
	bool readRom();
    void freeRom();
	void getCartridgeInformation();
    bool getColorGBFlag() const;
    int getMBCNum() const;
    bool isColorGB() const;
    std::string getGameTitle() const;

    std::vector<unsigned char> bios;
    std::shared_ptr<spdlog::logger> logger;
    std::vector<unsigned char> romBuffer;
    std::string cartridgeFilename;
    std::string biosFilename;
    std::string game_title_str;
    int num_ROM_banks;
    int num_RAM_banks;
    bool is_in_bios;
    bool has_bios;
    bool bios_is_cgb;
    unsigned char game_title_hash;
    uint16_t game_title_hash_16;

private:
    struct CartridgeType
    {
        int mbc;
        bool rom, ram, battery, timer, rumble, sensor, mmm01;
    };
    std::vector<unsigned char> readFile(const std::string& filename) const;
    int uncompressZip(const std::string& filename, std::vector<unsigned char>& uncompressed_out) const;
    int getNumOfRomBanks(unsigned char rom_size);
    int getNumOfRamBanks(unsigned char ram_size);
    void parseCartridgeType(unsigned char cartridge_type);
    void setMBC(unsigned char cartridge_type);
    void setROMSize(unsigned char cartridge_type);
    void setRAMSize(unsigned char cartridge_type);
    void setBattery(unsigned char cartridge_type);
    void setTimer(unsigned char cartridge_type);
    void setMMM01(unsigned char cartridge_type);
    void setRumble(unsigned char cartridge_type);
    bool endsWith(const std::string& str, const std::string& suffix) const;
    std::string getFileExtension(const std::string& filepath) const;
    bool romIsValid(const std::string& filepath) const;

	CartridgeType cartridgeType;

    // Information about the cartridge
    unsigned char game_title[16];
    unsigned char manufacturer_code[4];
    unsigned char new_licensee_code[2];
    unsigned char old_licensee_code;
    unsigned char cgb_flag;
    unsigned char sgb_flag;
    unsigned char cartridge_type;
    unsigned char rom_size;
    unsigned char ram_size;
    unsigned char destination_code;	// 0x00 = Japanese, 0x01 = non-Japanese
    unsigned char game_version;
    unsigned char header_checksum;	// Checks if cartridge is valid
    const bool force_cgb;
};

#endif