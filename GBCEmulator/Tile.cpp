
#include "stdafx.h"
#include "Tile.h"

Tile::Tile()
{

}

Tile::~Tile()
{

}

void Tile::updateRawData(std::uint8_t pos, std::uint8_t val)
{
	if (pos < 16)
	{
		raw_data[pos] = val;
		updatePixelRow(pos / 2);
	}
}

void Tile::updatePixelRow(std::uint8_t row_num)
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
			pixels[row_num][x] = color;
		}
	}
}

void Tile::getPixelRow(std::uint8_t row_num, unsigned char **row)
{
	if (row_num < 8)
	{
		*row = pixels[row_num];
	}
	else
	{
		*row = NULL;
	}
}