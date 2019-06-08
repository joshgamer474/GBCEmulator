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
	CartridgeReader(std::shared_ptr<spdlog::logger> logger);
	virtual ~CartridgeReader();
    CartridgeReader& operator=(const CartridgeReader& rhs);

	void setRomDestination(std::string filename);
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
    std::string game_title_str;
    int num_ROM_banks;
    int num_RAM_banks;
    bool is_in_bios;
    bool has_bios;

private:
    struct CartridgeType
    {
        int mbc;
        bool rom, ram, battery, timer, rumble, sensor, mmm01;
    };

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

	CartridgeType cartridgeType;

    // Information about the cartridge
    unsigned char game_title[16];
    unsigned char manufacturer_code[4];
    unsigned char cgb_flag;
    unsigned char sgb_flag;
    unsigned char cartridge_type;
    unsigned char rom_size;
    unsigned char ram_size;
    unsigned char destination_code;	// 0x00 = Japanese, 0x01 = non-Japanese
    unsigned char game_version;
    unsigned char header_checksum;	// Checks if cartridge is valid
};

#endif