
#include "stdafx.h"
#include "Tile.h"

Tile::Tile()
{
    raw_data.resize(16);
    pixels.resize(8 * 8);
}

Tile::~Tile()
{

}

void Tile::updateRawData(uint8_t pos, uint8_t val)
{
	if (pos < 16)
	{
		raw_data[pos] = val;
		updatePixelRow(pos / 2);
	}
}

void Tile::updatePixelRow(uint8_t row_num)
{
	if (row_num < 8)
	{
		uint8_t first_byte_pos = row_num * 2;
		uint8_t first_byte = raw_data[first_byte_pos];
		uint8_t second_byte = raw_data[first_byte_pos + 1];
		uint8_t color;

		for (int x = 0; x < 8; x++)
		{
			color = ((second_byte >> (7 - x)) & 0x01) << 1;
			color += ((first_byte >> (7 - x)) & 0x01);
			pixels[x + (row_num * 8)] = color;
		}
	}
}

uint8_t Tile::getPixel(uint8_t row, uint8_t column)
{
    if (row < 8 && column < 8)
    {
        return pixels[column + (row * 8)];
    }
    else
    {
        return 0;
    }
}

std::vector<uint8_t> Tile::getRawPixelData()
{
    return pixels;
}