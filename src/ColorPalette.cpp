#include "ColorPalette.h"

ColorPalette::ColorPalette()
{

}

ColorPalette::~ColorPalette()
{

}

void ColorPalette::updateRawByte(uint8_t pos, uint8_t data)
{
    raw_data[pos] = data;

    updateColor(pos);
}

void ColorPalette::updateColors()
{
    for (int i = 0; i < raw_data.size(); i += 2)
    {   // Get 2 byte raw data for color
        uint16_t twoBytes = raw_data[i];
        twoBytes |= (raw_data[i + 1] << 8);

        // Get color
        SDL_Color & color = colors[i / 2];

        // Set color
        color.r = twoBytes & 0x1F;
        color.g = twoBytes & 0x03E0;
        color.b = twoBytes & 0x7C00;
    }
}

void ColorPalette::updateColor(uint8_t bytePos)
{
    if (bytePos % 2 == 1)
    {   // bytePos is odd, make it even
        bytePos--;
    }

    // Get 2 byte raw data for color
    uint16_t twoBytes = raw_data[bytePos];
    twoBytes |= (raw_data[bytePos + 1] << 8);

    // Get color
    SDL_Color & color = colors[bytePos / 2];

    // Set color
    color.r = twoBytes & 0x1F;
    color.g = twoBytes & 0x03E0;
    color.b = twoBytes & 0x7C00;
}

SDL_Color ColorPalette::getColor(uint8_t index)
{
    return colors[index];
}