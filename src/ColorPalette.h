#ifndef COLOR_PALETTE_H
#define COLOR_PALETTE_H

#include <array>
#include <SDL.h>

class ColorPalette
{
public:
    ColorPalette();
    virtual ~ColorPalette();

    void updateRawByte(uint8_t pos, uint8_t data);
    SDL_Color getColor(uint8_t index);

private:
    void updateColors();
    void updateColor(uint8_t bytePos);

    std::array<uint8_t, 8> raw_data;
    std::array<SDL_Color, 4> colors;
};

#endif