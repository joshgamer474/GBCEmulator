#include "ColorPalette.h"

ColorPalette::ColorPalette()
{

}

ColorPalette::~ColorPalette()
{

}

void ColorPalette::updateRawByte(const uint8_t & pos, const uint8_t & data)
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
        setColor(twoBytes, color);
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
    setColor(twoBytes, color);
}

void ColorPalette::setColor(const uint16_t & data, SDL_Color & color)
{
    // Set color
    color.r = data & 0x1F;
    color.g = (data & 0x03E0) >> 5;
    color.b = (data & 0x7C00) >> 10;
    color.a = 0xFF;

    // Scale colors from 0x00 - 0x1F to 0x00 - 0xFF
    color.r *= 8;
    color.g *= 8;
    color.b *= 8;
}

SDL_Color ColorPalette::getColor(const uint8_t & index)
{
    return colors[index];
}