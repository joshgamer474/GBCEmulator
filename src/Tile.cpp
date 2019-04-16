
#include "stdafx.h"
#include "Tile.h"

Tile::Tile()
    : color_palette(NULL)
{
    raw_data.resize(16);
    pixels.resize(8 * 8);
    cgb_bg_palette_num = 0;
}

Tile::~Tile()
{

}

void Tile::updateRawData(const uint8_t & pos, const uint8_t & val)
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

uint8_t Tile::getPixel(const uint8_t & row, const uint8_t & column) const
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

const std::vector<uint8_t>& Tile::getRawPixelData() const
{
    return pixels;
}

void Tile::setCGBAttribute(uint8_t attribute_byte)
{
    cgb_attribute = attribute_byte;
    parseCGBAttribute();
}

void Tile::parseCGBAttribute()
{
    cgb_bg_palette_num      = cgb_attribute & 0x07;
    cgb_tile_vram_bank_num  = cgb_attribute & 0x08;
    // Bit 4 not used
    horizontal_flip         = cgb_attribute & 0x20;
    vertical_flip           = cgb_attribute & 0x40;
    bg_to_OAM_priority      = cgb_attribute & 0x80;
}

void Tile::setCGBColorPalette(ColorPalette * colorPalette)
{
    color_palette = colorPalette;
}

ColorPalette * Tile::getCGBColorPalette()
{
    return color_palette;
}