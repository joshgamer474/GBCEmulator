#include <GetUniqueColorPalette.h>

// https://tcrf.net/Notes:Game_Boy_Color_Bootstrap_ROM#Manual_Select_Palette_Configurations
// Returns GB color palette based on game's title hash
CGBROMPalette GetUniqueGBPalette(const uint8_t& hash)
{
    Palette bg, obj0, obj1;

    switch (hash)
    {   // Table 0
        case 0x71:
        case 0xFF:
            obj0 =  { WHTCLR_U32, 0xFF9C00FF, RED_U32, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0xFF9C00FF, RED_U32, BLACK_U32 };
            bg =    { WHITE_U32,  0xFF9C00FF, RED_U32, BLACK_U32 };
            break;

        case 0x15:
        case 0xDB:
            obj0 =  { WHTCLR_U32, YELLOW_U32, RED_U32, BLACK_U32 };
            obj1 =  { WHTCLR_U32, YELLOW_U32, RED_U32, BLACK_U32 };
            bg =    { WHITE_U32,  YELLOW_U32, RED_U32, BLACK_U32 };
            break;

        case 0x88:
            obj0 =  { 0xA59CFF00, YELLOW_U32, 0x006300FF, BLACK_U32 };
            obj1 =  { 0xA59CFF00, YELLOW_U32, 0x006300FF, BLACK_U32 };
            bg =    { 0xA59CFFFF, YELLOW_U32, 0x006300FF, BLACK_U32 };
            break;

        case 0x0C:
        case 0x16:
        case 0x35:
        case 0x67:
        case 0x75:
        case 0x92:
        case 0x99:
        case 0xB7:
            obj0 =  { WHTCLR_U32, 0xFFAD63FF, 0x843100FF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0xFFAD63FF, 0x843100FF, BLACK_U32 };
            bg =    { WHITE_U32,  0xFFAD63FF, 0x843100FF, BLACK_U32 };
            break;

        case 0x28:
        case 0x41:
        case 0xA5:
        case 0xE8:
            obj0 =  { BLKCLR_U32, 0x008484FF, 0xFFDE00FF, WHITE_U32 };
            obj1 =  { BLKCLR_U32, 0x008484FF, 0xFFDE00FF, WHITE_U32 };
            bg =    { BLACK_U32,  0x008484FF, 0xFFDE00FF, WHITE_U32 };
            break;

        case 0x58:
            obj0 =  { WHTCLR_U32, 0xA5A5A5FF, 0x525252FF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0xA5A5A5FF, 0x525252FF, BLACK_U32 };
            bg =    { WHITE_U32,  0xA5A5A5FF, 0x525252FF, BLACK_U32 };
            break;

        case 0x6F:
            obj0 =  { WHTCLR_U32, 0xFFCE00FF, 0x9C6300FF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0xFFCE00FF, 0x9C6300FF, BLACK_U32 };
            bg =    { WHITE_U32,  0xFFCE00FF, 0x9C6300FF, BLACK_U32 };
            break;

        // Table 1
        case 0x8C:
            obj0 =  { WHTCLR_U32, 0xFF7300FF, 0x944200FF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0xADAD84FF, 0x42737BFF, BLACK_U32 };
            bg =    { WHITE_U32,  0xADAD84FF, 0x42737BFF, BLACK_U32 };
            break;

        case 0x61:
        case 0x45:
            obj0 =  { WHTCLR_U32, 0xFF8484FF, 0x943A3AFF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0x63A5FFFF, BLUE_U32,   BLACK_U32 };
            bg =    { WHITE_U32,  0x63A5FFFF, BLUE_U32,   BLACK_U32 };
            break;
        
        // case 0xD3:
        // case 0x52:
        //     obj0 =  { WHTCLR_U32, 0xFF8484FF, 0x943A3AFF, BLACK_U32 };
        //     obj1 =  { WHTCLR_U32, 0x8C8CDEFF, 0x52528CFF, BLACK_U32 };
        //     bg =    { WHITE_U32,  0x8C8CDEFF, 0x52528CFF, BLACK_U32 };
        //     break;

        case 0x14:
            obj0 =  { WHTCLR_U32, 0x7BFF31FF, 0x008400FF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0xFF8484FF, 0x943A3AFF, BLACK_U32 };
            bg =    { WHITE_U32,  0xFF8484FF, 0x943A3AFF, BLACK_U32 };
            break;

        case 0xAA:
            obj0 =  { WHTCLR_U32, 0xFF8484FF, 0x943A3AFF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0x7BFF31FF, 0x0063C5FF, BLACK_U32 };
            bg =    { WHITE_U32,  0x7BFF31FF, 0x0063C5FF, BLACK_U32 };
            break;

        // Table 2
        case 0x3C:
            obj0 =  { WHTCLR_U32, 0x63A5FFFF, BLUE_U32,   BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0xFF8484FF, 0x943A3AFF, BLACK_U32 };
            bg =    { WHITE_U32,  0x63A5FFFF, BLUE_U32,   BLACK_U32 };
            break;

        case 0x9C:
            obj0 =  { WHTCLR_U32, 0x8C8CDEFF, 0x52528CFF, BLACK_U32 };
            obj1 =  { 0xFFC54200, 0xFFD600FF, 0x943A00FF, BLACK_U32 };
            bg =    { WHITE_U32,  0x8C8CDEFF, 0x52528CFF, BLACK_U32 };
            break;

        // Table 3
        case 0xB3:
        case 0x55:
            obj0 =  { WHTCLR_U32, 0xFF7300FF, 0x944200FF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0xFF7300FF, 0x944200FF, BLACK_U32 };
            bg =    { WHITE_U32,  0xADAD84FF, 0x42737BFF, BLACK_U32 };
            break;

        case 0x34:
        case 0x66:
        //case 0x45:
        case 0xF4:
        case 0x20:
            obj0 =  { WHTCLR_U32, 0xFF8484FF, 0x943A3AFF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0xFF8484FF, 0x943A3AFF, BLACK_U32 };
            bg =    { WHITE_U32,  0x7BFF00FF, 0xB57300FF, BLACK_U32 };
            break;

        case 0x3D:
        case 0x6A:
        case 0x49:
            obj0 =  { WHTCLR_U32, 0xFF8484FF, 0x943A3AFF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0xFF8484FF, 0x943A3AFF, BLACK_U32 };
            bg =    { WHITE_U32,  0x52FF00FF, 0xFF4200FF, BLACK_U32 };
            break;

        case 0x19:
            obj0 =  { WHTCLR_U32, 0xFF8484FF, 0x943A3AFF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0xFF8484FF, 0x943A3AFF, BLACK_U32 };
            bg =    { WHITE_U32,  0xFF9C00FF, RED_U32,    BLACK_U32 };
            break;

        case 0x1D:  // Kirby's Pinball Land
            obj0 =  { 0xFF635200, 0xD60000FF, 0x630000FF, BLACK_U32 };
            obj1 =  { 0xFF635200, 0xD60000FF, 0x630000FF, BLACK_U32 };
            bg =    { 0xA59CFFFF, YELLOW_U32, 0x006300FF, BLACK_U32 };
            break;

        case 0x46:  // Super Mario Land (World)
        //case 0x45:
            obj0 =  { BLKCLR_U32, WHITE_U32,  0xFF8484FF, 0x943A3AFF };
            obj1 =  { BLKCLR_U32, WHITE_U32,  0xFF8484FF, 0x943A3AFF };
            bg =    { 0xB5B5FFFF, 0xFFFF94FF, 0xAD5A42FF, BLACK_U32 };
            break;

        case 0x0D:  // Pocket Bomberman (Europe)
        //case 0x45:
            obj0 =  { 0xFFC54200, 0xFFD600FF, 0x943A00FF, 0x4A0000FF };
            obj1 =  { 0xFFC54200, 0xFFD600FF, 0x943A00FF, 0x4A0000FF };
            bg =    { WHITE_U32, 0x8C8CDEFF, 0x52528CFF, BLACK_U32 };
            break;

        case 0xBF:
            obj0 =  { WHTCLR_U32, 0xFF8484FF, 0x943A3AFF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0xFF8484FF, 0x943A3AFF, BLACK_U32 };
            bg =    { WHITE_U32,  0x8C8CDEFF, 0x52528CFF, BLACK_U32 };
            break;

        //case 0x28:
        case 0x4B:
        case 0x90:
        case 0x9A:
        case 0xBD:
            obj0 =  { WHTCLR_U32, 0xFF8484FF, 0x943A3AFF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0xFF8484FF, 0x943A3AFF, BLACK_U32 };
            bg =    { WHITE_U32,  0x7BFF31FF, 0x008400FF, BLACK_U32 };
            break;

        case 0x39:
        case 0x43:
        case 0x97:
            obj0 =  { WHTCLR_U32, 0x63A5FFFF, BLUE_U32, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0x63A5FFFF, BLUE_U32, BLACK_U32 };
            bg =    { WHITE_U32,  0xFFAD63FF, 0x843100FF, BLACK_U32 };
            break;

        // case 0xA5: // Battletoads in Ragnarok's World (Europe)
        //     obj0 =  { WHTCLR_U32, 0x7BFF31FF, 0x008400FF, BLACK_U32 };
        //     obj1 =  { WHTCLR_U32, 0x7BFF31FF, 0x008400FF, BLACK_U32 };
        //     bg =    { WHITE_U32,  0xFFAD63FF, 0x843100FF, BLACK_U32 };
        //     break;

        case 0x00:
        case 0x18:
        case 0x3F:
        //case 0x66:
        case 0xC6:
            obj0 =  { WHTCLR_U32, 0xFF8484FF, 0x943A3AFF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0xFF8484FF, 0x943A3AFF, BLACK_U32 };
            bg =    { WHITE_U32,  0x7BFF31FF, 0x0063C5FF, BLACK_U32 };
            break;

        // Table 4
        case 0x95:
        //case 0xB3:
        //case 0x52:
            obj0 =  { WHTCLR_U32, 0x52FF00FF, 0xFF4200FF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0x5ABDFFFF, RED_U32, BLUE_U32 };
            bg =    { WHITE_U32,  0x52FF00FF, 0xFF4200FF, BLACK_U32 };
            break;

        case 0x3E:
        case 0xE0:
            obj0 =  { WHTCLR_U32, 0xFF9C00FF, RED_U32, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0x5ABDFFFF, RED_U32, BLUE_U32 };
            bg =    { WHITE_U32,  0xFF9C00FF, 0xFF4200FF, BLACK_U32 };
            break;

        //case 0x0D:
        case 0x69:
        case 0xF2:
            obj0 =  { WHTCLR_U32, YELLOW_U32, RED_U32, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0x5ABDFFFF, RED_U32, BLUE_U32 };
            bg =    { WHITE_U32,  YELLOW_U32, 0xFF4200FF, BLACK_U32 };
            break;

        // Table 5
        case 0x59:
        //case 0xC6:  // Wario Land - Super Mario Land 3 (World)
            obj0 =  { WHTCLR_U32, 0xFF7300FF, 0x944200FF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0x5ABDFFFF, RED_U32, BLUE_U32 };
            bg =    { WHITE_U32,  0xADAD84FF, 0x42737BFF, BLACK_U32 };
            break;

        case 0x86:  // Donkey Kong Land
        case 0xA8:
            obj0 =  { 0xFFC54200, 0xFFD600FF, 0x943A00FF, 0x4A0000FF };
            obj1 =  { WHTCLR_U32, 0xFF8484FF, 0x943A3AFF, BLACK_U32 };
            bg =    { 0xFFFF9CFF, 0x94B5FFFF, 0x639473FF, 0x003A3AFF };
            break;

        //case 0xBF:
        case 0xCE:
        case 0xD1:
        case 0xF0:  // Tennis
            obj0 =  { WHTCLR_U32, WHITE_U32, 0x63A5FFFF, BLUE_U32 };
            obj1 =  { WHTCLR_U32, 0xFFAD63FF, 0x843100FF, BLACK_U32 };
            bg =    { 0x6BFF00FF, WHITE_U32, 0xFF524AFF, BLACK_U32 };
            break;

        case 0x36:  // Baseball (World)
            obj0 =  { WHTCLR_U32, WHITE_U32, 0x63A5FFFF, BLUE_U32 };
            obj1 =  { WHTCLR_U32, 0xFF8484FF, 0x943A3AFF, BLACK_U32 };
            bg =    { 0x52DE00FF, 0xFF8400FF, YELLOW_U32, WHITE_U32 };
            break;

        case 0x27:  // Kirby
        //case 0x49:
        case 0x5C:
        //case 0xB3:
            obj0 =  { 0xFF635200, 0xD60000FF, 0x630000FF, BLACK_U32 };
            obj1 =  { 0x0000FF00, WHITE_U32, 0xFFFF7BFF, 0x0084FFFF };
            bg =    { 0xA59CFFFF, YELLOW_U32, 0x006300FF, BLACK_U32 };
            break;

        case 0xC9:  // Super Mario Land 2
            obj0 =  { WHTCLR_U32, 0xFF7300FF, 0x944200FF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0x63A5FFFF, BLUE_U32, 0x0084FFFF };
            bg =    { 0xFFFFCEFF, 0x63EFEFFF, 0x9C8431FF, 0x5A5A5AFF };
            break;

        case 0x4E:  // Wave Race
            obj0 =  { WHTCLR_U32, 0xFF8484FF, 0x943A3AFF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0xFFFF7BFF, 0x0084FFFF, RED_U32 };
            bg =    { WHITE_U32, 0x63A5FFFF, BLUE_U32, BLACK_U32 };
            break;

        //case 0x18:  // Donkey Kong Land 1/2/3
        //case 0x6A:
        case 0x6B:
            obj0 =  { 0xFFC54200, 0xFFD600FF, 0x943A00FF, 0x4A0000FF };
            obj1 =  { WHTCLR_U32, 0x5ABDFFFF, RED_U32, BLUE_U32 };
            bg =    { WHITE_U32, 0x8C8CDEFF, 0x52528CFF, BLACK_U32 };
            break;

        case 0x9D:  // Killer Instinct
            obj0 =  { WHTCLR_U32, 0xFF8484FF, 0x943A3AFF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0xFFAD63FF, 0x843100FF, BLACK_U32 };
            bg =    { WHITE_U32, 0x8C8CDEFF, 0x52528CFF, BLACK_U32 };
            break;

        case 0x17:  // Mystic Quest
        //case 0x27:
        //case 0x61:
        case 0x8B:
            obj0 =  { WHTCLR_U32, 0xFF8484FF, 0x943A3AFF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0x63A5FFFF, BLUE_U32, BLACK_U32 };
            bg =    { WHITE_U32, 0x7BFF31FF, 0x008400FF, BLACK_U32 };
            break;

        case 0x01:
        case 0x10:
        case 0x29:
        case 0x52:
        case 0x5D:
        case 0x68:
        case 0x6D:
        case 0xF6: // Mega Man 2/3
            obj0 =  { WHTCLR_U32, 0x63A5FFFF, BLUE_U32, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0x7BFF31FF, 0x008400FF, BLACK_U32 };
            bg =    { WHITE_U32, 0xFFAD63FF, 0x843100FF, BLACK_U32 };
            break;

        case 0x70: // Legend of Zelda - The Link's Awakening
            obj0 =  { WHTCLR_U32, 0x00FF00FF, 0x318400FF, 0x004A00FF };
            obj1 =  { WHTCLR_U32, 0x63A5FFFF, BLUE_U32, BLACK_U32 };
            bg =    { WHITE_U32, 0xFF8484FF, 0x943A3AFF, BLACK_U32 };
            break;

        case 0xA2: // Boy and His Blob
        case 0xF7: // Star Wars
            obj0 =  { WHTCLR_U32, 0x7BFF31FF, 0x008400FF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0x63A5FFFF, BLUE_U32, BLACK_U32 };
            bg =    { WHITE_U32, 0xFFAD63FF, 0x843100FF, BLACK_U32 };
            break;

        //case 0x46: // Metroid 2
            // obj0 =  { 0xFFFF0000, RED_U32, 0x630000FF, BLACK_U32 };
            // obj1 =  { WHTCLR_U32, 0x7BFF31FF, 0x008400FF, BLACK_U32 };
            // bg =    { WHITE_U32, 0x63A5FFFF, BLUE_U32, BLACK_U32 };
            // break;

        case 0xD3: // Wario Land 2
            obj0 =  { WHTCLR_U32, 0xFFAD63FF, 0x843100FF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0x63A5FFFF, BLUE_U32, BLACK_U32 };
            bg =    { WHITE_U32, 0xADAD84FF, 0x843100FF, BLACK_U32 };
            break;

        default:
            obj0 =  { WHTCLR_U32, 0xA5A5A5FF, 0x525252FF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0xA5A5A5FF, 0x525252FF, BLACK_U32 };
            bg =    { WHITE_U32,  0xA5A5A5FF, 0x525252FF, BLACK_U32 };
    }

    return CGBROMPalette { obj0, obj1, bg };
}