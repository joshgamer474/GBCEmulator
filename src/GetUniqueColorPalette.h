#ifndef SRC_GET_UNIQUE_COLOR_PALETTE_H
#define SRC_GET_UNIQUE_COLOR_PALETTE_H

#include <array>

#define WHTCLR_U32  (uint32_t) 0xFFFFFF00   // White with clear alpha
#define BLKCLR_U32  (uint32_t) 0x00000000   // Black with clear alpha
#define WHITE_U32   (uint32_t) 0xFFFFFFFF
#define BLACK_U32   (uint32_t) 0x000000FF
#define YELLOW_U32  (uint32_t) 0xFFFF00FF
#define RED_U32     (uint32_t) 0xFF0000FF
#define BLUE_U32    (uint32_t) 0x0000FFFF

struct Palette {
    std::array<uint32_t, 4> colors; // RGBA * 4
};

struct CGBROMPalette {
    Palette obj0;
    Palette obj1;
    Palette bg;
};

CGBROMPalette GetUniqueGBPalette(const uint8_t& hash);

#endif // SRC_GET_UNIQUE_COLOR_PALETTE_H