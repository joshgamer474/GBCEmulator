#pragma once

#ifndef GPU_H
#define GPU_H

#include <vector>

#define VRAM_SIZE 0x2000
#define OAM_SIZE 0xA0

class GPU
{

private:

	std::vector<std::vector<unsigned char>> vram_banks;
	std::vector<unsigned char> object_attribute_memory;

public:

	GPU();
	~GPU();

	bool is_color_gb;
	int num_vram_banks;	// 1 for regular GB, 2 for GB color
	int curr_vram_bank;

	// Reading and writing methods
	std::uint8_t readByte(std::uint16_t pos);
	void setByte(std::uint16_t pos, std::uint8_t val);
};
#endif