#ifndef TILE_H
#define TILE_H

#include <cstdint>
#include <vector>
#include <ColorPalette.h>

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
    void setCGBAttribute(uint8_t attribute_byte);
    void setCGBColorPalette(ColorPalette * color_palette);
    ColorPalette * getCGBColorPalette();

    std::vector<uint8_t> pixels;

private:
    void updatePixelRow(uint8_t row_num);
    void parseCGBAttribute();

    std::vector<uint8_t> raw_data;

    // CGB variables
    uint8_t cgb_attribute;
    uint8_t cgb_bg_palette_num;
    bool cgb_tile_vram_bank_num;
    bool horizontal_flip;
    bool vertical_flip;
    bool bg_to_OAM_priority;
    ColorPalette * color_palette;
};


#endif