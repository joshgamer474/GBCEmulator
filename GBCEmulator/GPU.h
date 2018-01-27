#pragma once

#ifndef GPU_H
#define GPU_H

#include <vector>

#define VRAM_SIZE 0x2000
#define OAM_SIZE 0xA0
#define PALETTE_DATA_SIZE 0x08

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

	/*
		LCD Registers
	*/
	unsigned char lcd_control;
	unsigned char lcd_status;

	unsigned char scroll_y, scroll_x, lcd_y, lcd_y_compare, window_y_pos, window_x_pos;

	// LCD Monochrome Palettes
	unsigned char bg_palette, object_pallete0, object_pallete1;

	// LCD Color Palettes (GBC only)
	unsigned char background_palette_index, sprite_palette_index;
	std::vector<unsigned char> background_palette_data;
	std::vector<unsigned char> sprite_palette_data;

	// LCD Object Attribute Memory DMA Transfers
	unsigned char oam_dma;

	// LCD VRAM DMA Transfers (GBC only)
	unsigned char hdma1, hdma2, hdma3, hdma4, hdma5;

	// Reading and writing methods
	std::uint8_t readByte(std::uint16_t pos);
	void setByte(std::uint16_t pos, std::uint8_t val);
};
#endif