#pragma once

#ifndef TILE_H
#define TILE_H
#include <cstdint>

#define NUM_BG_TILES_PER_BLOCK 128
#define NUM_BG_TILE_BLOCKS 3
#define NUM_BYTES_PER_TILE 16

class Tile
{
private:

	unsigned char raw_data[16];
	unsigned char pixels[8][8];

public:

	Tile();
	~Tile();

	void updateRawData(std::uint8_t pos, std::uint8_t val);
	void updatePixelRow(std::uint8_t row_num);
	void getPixelRow(std::uint8_t row_num, unsigned char **row);

};


#endif