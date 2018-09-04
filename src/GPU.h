#pragma once

#ifndef GPU_H
#define GPU_H

#include <vector>
#include <SDL.h>
#include <spdlog/spdlog.h>

#define VRAM_SIZE 0x2000
#define OAM_SIZE 0xA0
#define PALETTE_DATA_SIZE 0x08

#define SCREEN_PIXEL_H 144
#define SCREEN_PIXEL_W 160
#define SCREEN_FRAMERATE 60

class Memory;
class CPU;
class Tile;

class GPU
{

private:
    bool SDLColorsAreEqual(const SDL_Color & a, const SDL_Color & b);

	std::vector<std::vector<unsigned char>> vram_banks;
	std::vector<unsigned char> object_attribute_memory;
	std::vector<std::vector<Tile>> bg_tiles;

public:

	GPU(SDL_Renderer *render);
	~GPU();

	void init_gbc();

	std::shared_ptr<CPU> cpu;
    std::shared_ptr<Memory> memory;
	bool is_color_gb;
	int num_vram_banks;	// 1 for regular GB, 2 for GB color
	int curr_vram_bank;
	std::uint64_t ticks, last_ticks;

	SDL_Color frame[SCREEN_PIXEL_W * SCREEN_PIXEL_H];
	SDL_Color bg_palette_color[4];
	SDL_Color object_palette0_color[4];
	SDL_Color object_palette1_color[4];
	void set_color_palette(SDL_Color *palette, std::uint8_t val, bool zero_is_transparent = false);


	struct TILE
	{
		unsigned char b0, b1;
	};
	void getTile(int tile_num, int line_num, TILE *tile);
	std::uint8_t y_roll_over;

	enum GPU_MODE
	{
		GPU_MODE_HBLANK,
		GPU_MODE_VBLANK,
		GPU_MODE_OAM,
		GPU_MODE_VRAM,
		GPU_MODE_NONE
	};
	int gpu_mode;

	void init_color_gb();
	void run();
	void renderLine();
	void renderLineTwo();
	void display();
	void updateTile(std::uint16_t pos, std::uint8_t val, std::uint8_t tile_block_num);
    void set_lcd_control(unsigned char lcd_control);
    void set_lcd_status(unsigned char lcd_status);
    void set_lcd_status_mode_flag(GPU_MODE);
    void set_lcd_status_coincidence_flag(bool flag);
    void printFrame();
    const SDL_Color * getFrame();

    const std::vector<std::vector<Tile>> & getBGTiles();
    const std::vector<int> & getUpdatedBGTileIndexes();
    uint8_t getTileBlockNum(int use_tile_num);
    Tile * getTileFromBGTiles(uint8_t tile_block_num, int use_tile_num);

    // Reading and writing methods
    std::uint8_t readByte(std::uint16_t pos);
    void setByte(std::uint16_t pos, std::uint8_t val);


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
	unsigned char background_palette_index, sprite_palette_index;
	bool auto_increment_background_palette_index, auto_increment_sprite_palette_index;
	std::vector<unsigned char> background_palette_data;
	std::vector<unsigned char> sprite_palette_data;

	// LCD Object Attribute Memory DMA Transfers
	unsigned char oam_dma;

	// LCD VRAM DMA Transfers (GBC only)
	unsigned char hdma1, hdma2, hdma3, hdma4, hdma5;


	/*
		LCD methods
	*/
	struct LCDSelect
	{
		std::uint16_t start, end;
	};

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


	/*
		Input handling
	*/
	SDL_Event e;

	/*
		Graphics
	*/
	SDL_Renderer *renderer;
	SDL_Texture *game_screen;


	std::shared_ptr<spdlog::logger> logger;

    bool frame_is_ready;
    bool bg_tiles_updated;
};
#endif