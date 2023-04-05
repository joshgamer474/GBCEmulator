#ifndef GPU_H
#define GPU_H

#include <array>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <SDL.h>
#include "ColorPalette.h"
#include <GetUniqueColorPalette.h>
#include <spdlog/spdlog.h>

#define VRAM_SIZE 0x2000
#define OAM_SIZE 0xA0
#define OAM_NUM_SPRITES OAM_SIZE / 4
#define PALETTE_DATA_SIZE 4
#define CGB_PALETTE_DATA_SIZE 8
#define CGB_PALETTE_DATA_SIZE_RAW 64

#define SCREEN_PIXEL_H 144
#define SCREEN_PIXEL_W 160
#define SCREEN_FRAMERATE 60
#define SCREEN_PIXEL_TOTAL SCREEN_PIXEL_H * SCREEN_PIXEL_W

#define TOTAL_SCREEN_PIXEL_H 256
#define TOTAL_SCREEN_PIXEL_W 256
#define TOTAL_SCREEN_TILE_H 32
#define TOTAL_SCREEN_TILE_W 32

class Memory;
class Tile;

struct TILE {
    unsigned char b0, b1;
};

struct LCDSelect {
    uint16_t start, end;
};

enum class CGBPaletteCombo : int {
    NONE,
    UP,
    UP_A,
    UP_B,
    LEFT,
    LEFT_A,
    LEFT_B,
    DOWN,
    DOWN_A,
    DOWN_B,
    RIGHT,
    RIGHT_A,
    RIGHT_B,
    CUSTOM,
    LAST,
};

class GPU
{
public:
    enum GPU_MODE
    {
        GPU_MODE_HBLANK,
        GPU_MODE_VBLANK,
        GPU_MODE_OAM,
        GPU_MODE_VRAM,
        GPU_MODE_NONE
    };

    GPU(std::shared_ptr<spdlog::logger> logger);
    ~GPU();
    GPU& operator=(const GPU& rhs);

    void init_color_gb();
    void run(const uint8_t & cpuTicks);
    std::array<SDL_Color, SCREEN_PIXEL_TOTAL> getFrame() const;
    uint8_t readByte(const uint16_t& pos, const bool limit_access = true) const;
    void setByte(const uint16_t& pos, const uint8_t& val, const bool limit_access = true);
    std::vector<std::vector<std::vector<Tile>>>& getBGTiles();
    const std::vector<int>& getUpdatedBGTileIndexes();
    void changeCGBPalette();

    std::shared_ptr<Memory> memory;
    std::shared_ptr<spdlog::logger> logger;
    SDL_Color curr_frame[SCREEN_PIXEL_W * SCREEN_PIXEL_H];
    std::array<ColorPalette, 8> cgb_background_palettes;
    std::array<ColorPalette, 8> cgb_sprite_palettes;
    std::vector<SDL_Color> bg_frame;
    SDL_Color bg_palette_color[PALETTE_DATA_SIZE];
    SDL_Color object_palette0_color[PALETTE_DATA_SIZE];
    SDL_Color object_palette1_color[PALETTE_DATA_SIZE];
    int gpu_mode;
    bool frame_is_ready;
    bool bg_tiles_updated;
    bool render_full_frame;
    bool is_cgb_tile_palette_updated;
    bool is_tile_palette_updated;
    bool cgb_dma_in_progress;
    bool cgb_dma_hblank_in_progress;
    bool is_color_gb;

private:
    void set_color_palette(SDL_Color* palette, const uint8_t& val, bool zero_is_transparent = false);
    void use_color_palette(const CGBROMPalette& wanted_palette);
    SDL_Color get_sdl_color(const uint32_t& color) const;
    void renderLine();
    void drawBackgroundLine();
    void drawWindowLine();
    void drawOAMLine();
    uint16_t getTileMapNumber(const uint8_t& pixel_x, const uint8_t& pixel_y) const;
    void renderFullBackgroundMap();
    void drawShownBackgroundArea();
    Tile * updateTile(const uint16_t& pos, const uint8_t& val, const bool& use_vram_bank, const uint8_t& tile_block_num);
    void set_lcd_control(const uint8_t& lcd_control);
    void set_lcd_status(const uint8_t& lcd_status);
    void set_lcd_status_mode_flag(const GPU_MODE& mode);
    std::string getGPUModeStr(const GPU_MODE& mode) const;
    void update_lcd_status_coincidence_flag();
    void printFrame();
    CGBROMPalette get_cgb_rom_palette(const uint8_t& ref) const;
    uint8_t get_cgb_rom_palette_ref(const CGBPaletteCombo& ref) const;

    uint8_t getTileBlockNum(const int& use_tile_num) const;
    uint8_t getSpriteTileBlockNum(const int& use_tile_num) const;
    Tile * getTileFromBGTiles(const uint8_t& use_vram_bank, const uint8_t& tile_block_num, const int& use_tile_num);

    // Palette methods
    void updateBackgroundPalette(const uint8_t& val);
    void updateSpritePalette(const uint8_t& val);

    bool SDLColorsAreEqual(const SDL_Color & a, const SDL_Color & b);
    void sortNonCGBOAMSpriteOrder();

    int num_vram_banks;
    int curr_vram_bank;
    uint16_t ticks_accumulated;

    uint8_t y_roll_over;

    /*
    LCD Registers
    */
    unsigned char lcd_control;
    unsigned char lcd_status;
    unsigned char scroll_y, scroll_x;			// X, Y position of background to start drawing the viewing area from
    unsigned char lcd_y, lcd_y_compare;
    unsigned char window_y_pos, window_x_pos;	// X, Y position of viewing area to start drawing the window from. window_x_pos needs to be subtracted by 7

    // LCD Monochrome Color Palettes
    unsigned char bg_palette, object_pallete0, object_pallete1;

    // LCD Color Palettes (GBC only)
    unsigned char cgb_background_palette_index, cgb_sprite_palette_index;
    bool cgb_auto_increment_background_palette_index, cgb_auto_increment_sprite_palette_index;
    std::array<unsigned char, CGB_PALETTE_DATA_SIZE_RAW> cgb_background_palette_data;
    std::array<unsigned char, CGB_PALETTE_DATA_SIZE_RAW> cgb_sprite_palette_data;
    std::array<bool, SCREEN_PIXEL_W> cgb_bg_to_oam_priority_array;
    std::array<ColorPalette *, SCREEN_PIXEL_W> cgb_bg_scanline_color_palettes;

    // LCD Object Attribute Memory DMA Transfers
    unsigned char oam_dma;

    // LCD VRAM DMA Transfers (GBC only)
    unsigned char hdma1, hdma2, hdma3, hdma4, hdma5;
    uint8_t cgb_dma_transfer_bytes_left;

    // LCD Control Register objects
    LCDSelect window_tile_map_display_select, bg_tile_data_select, bg_tile_map_select;
    bool bg_tile_data_select_method, bg_tile_map_select_method;
    std::uint8_t object_size;
    bool lcd_display_enable;
    bool window_display_enable;
    bool object_display_enable;
    bool bg_display_enable;

    // LCD Status Register objects
    bool enable_lcd_y_compare_interrupt;

    // Graphics
    bool lcd_status_interrupt_signal;
    bool wait_frame_to_render_window;
    std::mutex frame_mutex;

    SDL_Color frame[SCREEN_PIXEL_W * SCREEN_PIXEL_H];

    std::vector<std::vector<unsigned char>> vram_banks;
    std::vector<unsigned char> object_attribute_memory;
    std::vector<std::vector<std::vector<Tile>>> bg_tiles;
    std::vector<uint8_t> objects_pos_to_use;

    CGBPaletteCombo curr_opt_gb_palette;
};
#endif