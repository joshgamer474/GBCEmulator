#ifndef COLOR_PALETTE_H
#define COLOR_PALETTE_H

#include <array>
#include <SDL.h>

#define CGB_NUM_COLORS_PER_PALETTE 4

class ColorPalette
{
public:
    ColorPalette();
    virtual ~ColorPalette();

    void updateRawByte(const uint8_t & pos, const uint8_t & data);
    const SDL_Color& getColor(const uint8_t & index) const;
    const std::array<uint8_t, 8>& getRawData() const;

private:
    void updateColors();
    void updateColor(uint8_t bytePos);
    void setColor(const uint16_t & data, SDL_Color & color);

    std::array<uint8_t, 8> raw_data;
    std::array<SDL_Color, CGB_NUM_COLORS_PER_PALETTE> colors;
};

#endif