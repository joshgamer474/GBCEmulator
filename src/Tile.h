#pragma once

#ifndef TILE_H
#define TILE_H
#include <cstdint>
#include <vector>

#define NUM_BG_TILES_PER_BLOCK 128
#define NUM_BG_TILE_BLOCKS 3
#define NUM_BYTES_PER_TILE 16

class Tile
{
public:
	Tile();
	~Tile();

	void updateRawData(uint8_t pos, uint8_t val);
    uint8_t getPixel(uint8_t row, uint8_t column);
    std::vector<uint8_t> getRawPixelData();

    std::vector<uint8_t> pixels;

private:
    void updatePixelRow(uint8_t row_num);

    std::vector<uint8_t> raw_data;
};


#endif