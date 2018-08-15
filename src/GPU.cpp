#include "stdafx.h"

#include "GPU.h"
#include "Memory.h"
#include "CPU.h"
#include "Joypad.h"
#include "CartridgeReader.h"
#include "Debug.h"
#include "Tile.h"

GPU::GPU(SDL_Renderer *render)
{
	is_color_gb = false;
	num_vram_banks = 1;
	curr_vram_bank = 0;
	ticks = 0;

	lcd_control = NULL;

	vram_banks.resize(num_vram_banks, std::vector<unsigned char>(VRAM_SIZE, 0));
	object_attribute_memory.resize(OAM_SIZE);
	bg_tiles.resize(NUM_BG_TILE_BLOCKS, std::vector<Tile>(NUM_BG_TILES_PER_BLOCK));

	background_palette_data.resize(PALETTE_DATA_SIZE * PALETTE_DATA_SIZE);
	sprite_palette_data.resize(PALETTE_DATA_SIZE);

	gpu_mode = GPU_MODE_VRAM;

#ifdef SDL_DRAW
	renderer = render;
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	game_screen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, SCREEN_PIXEL_W, SCREEN_PIXEL_H);
#endif

	background_palette_index = 0;
	sprite_palette_index = 0;
	bg_tile_data_select_method = false;
	bg_tile_map_select_method = false;
	auto_increment_background_palette_index = false;
	auto_increment_sprite_palette_index = false;

    frame_is_ready = false;
    bg_tiles_updated = false;
}


GPU::~GPU()
{
    logger.reset();
}


void GPU::init_color_gb()
{
	num_vram_banks = 2;
	vram_banks.resize(num_vram_banks, std::vector<unsigned char>(VRAM_SIZE, 0));
}

std::uint8_t GPU::readByte(std::uint16_t pos)
{
	switch (pos & 0xF000)
	{
	case 0x8000:
	case 0x9000:

		return vram_banks[curr_vram_bank][pos - 0x8000];

	case 0xF000:

		if (pos < 0xFF00)
		{
			// 0xFE00 - 0xFE9F : Sprite RAM (OAM)
			return object_attribute_memory[pos - 0xFE00];
		}
		else if (pos < 0xFF6C)
		{
			// LCD Stuff, VRAM bank selector
			switch (pos)
			{
			case 0xFF40:	return lcd_control;
			case 0xFF41:	return lcd_status;
			case 0xFF42:	return scroll_y;
			case 0xFF43:	return scroll_x;
			case 0xFF44:	return lcd_y;
			case 0xFF45:	return lcd_y_compare;
			case 0xFF46:	return oam_dma;
			case 0xFF47:	return bg_palette;
			case 0xFF48:	return object_pallete0;
			case 0xFF49:	return object_pallete1;
			case 0xFF4A:	return window_y_pos;
			case 0xFF4B:	return window_x_pos;
			case 0xFF4F:	return curr_vram_bank;
			case 0xFF51:	return hdma1;
			case 0xFF52:	return hdma2;
			case 0xFF53:	return hdma3;
			case 0xFF54:	return hdma4;
			case 0xFF55:	return hdma5;
			case 0xFF68:	return background_palette_index;
			case 0xFF69:	return background_palette_data[background_palette_index];
			case 0xFF6A:	return sprite_palette_index;
			case 0xFF6B:	return sprite_palette_data[sprite_palette_index];

			default:
				logger->warn("GPU::readByte() doesn't handle address: 0x{0:x}", pos);
				return 0xFF;
			}
		}

	default:
		logger->warn("GPU::readByte() doesn't handle address: 0x{0:x}", pos);
		return 0xFF;
	}
}


void GPU::setByte(std::uint16_t pos, std::uint8_t val)
{
	std::uint8_t tile_block_num = 3;

	switch (pos & 0xF000)
	{
	case 0x8000:
	case 0x9000:

		vram_banks[curr_vram_bank][pos - 0x8000] = val;

		// Background Tile Data: 0x8000 - 0x97FF
		if (pos < 0x8800)
		{
			tile_block_num = 0;
		}
		else if (pos < 0x9000)
		{
			tile_block_num = 1;
		}
		else if (pos < 0x9800)
		{
			tile_block_num = 2;
		}

		if (tile_block_num < 3)
		{
			updateTile(pos, val, tile_block_num);
		}

		// Background Maps: 0x9800 - 0x9FFF (0x9800 - 0x9BFF, 0x9C00 - 0x9FFF)
		break;

	case 0xF000:

		if (pos < 0xFF00)
		{
			// 0xFE00 - 0xFE9F : Sprite RAM (OAM)
			object_attribute_memory[pos - 0xFE00] = val;
			break;
		}
		else if (pos < 0xFF6C)
		{
			// LCD Stuff, VRAM bank selector

			if (memory->cartridgeReader->getColorGBFlag())
			{
				switch (pos)
				{

				case 0xFF68:	background_palette_index = val & 0x3F; 
								auto_increment_background_palette_index = val & 0x80; 
								return;
				case 0xFF69:	background_palette_data[background_palette_index] = val; 
								if (auto_increment_background_palette_index) background_palette_index++;
								background_palette_index &= 0x3F;
								return;
				case 0xFF6A:	sprite_palette_index = val & 0x07; 
								auto_increment_sprite_palette_index = val & 0x80; 
								return;
				case 0xFF6B:	sprite_palette_data[sprite_palette_index] = val; 
								if (auto_increment_sprite_palette_index) sprite_palette_index++;
								sprite_palette_index &= 0x07;
								return;

				}
			}


			switch (pos)
			{
			case 0xFF40:	set_lcd_control(val); break;
			case 0xFF41:	lcd_status = val; break;
			case 0xFF42:	scroll_y = val; break;
			case 0xFF43:	scroll_x = val; break;
			case 0xFF44:	lcd_y = 0; break;				// Read only - Writing to this register resets the counter
			case 0xFF45:	lcd_y_compare = val; break;
			case 0xFF46:	memory->do_oam_dma_transfer(val); break;
			case 0xFF47:	bg_palette = val; set_color_palette(bg_palette_color, val); break;
			case 0xFF48:	object_pallete0 = val; set_color_palette(object_palette0_color, val); break;
			case 0xFF49:	object_pallete1 = val; set_color_palette(object_palette1_color, val); break;
			case 0xFF4A:	window_y_pos = val; break;
			case 0xFF4B:	window_x_pos = val; break;
			case 0xFF4F:	curr_vram_bank = (val & 0x01); break;
			case 0xFF51:	hdma1 = val; break;
			case 0xFF52:	hdma2 = val; break;
			case 0xFF53:	hdma3 = val; break;
			case 0xFF54:	hdma4 = val; break;
			case 0xFF55:	hdma5 = val; break;

			default:
				logger->warn("GPU::setByte() doesn't handle address: 0x{0:x}", pos);
			}
		}
		break;
	default:
		logger->warn("GPU::setByte() doesn't handle address: 0x{0:x}", pos);
	}
}


void GPU::set_color_palette(SDL_Color *palette, std::uint8_t val)
{
	unsigned char color_val = 0;

	for (int i = 0; i < 4; i++)
	{
		switch ((val >> (i * 2)) & 3)
		{
		case 0: color_val = 255; break;
		case 1: color_val = 192; break;
		case 2: color_val = 96; break;
		case 3: color_val = 0; break;
		}
		palette[i] = { color_val, color_val, color_val, 255 };
	}
}


void GPU::set_lcd_control(unsigned char lcdControl)
{
	lcd_control = lcdControl;
	bool old_lcd_display_enable = lcd_display_enable;

	lcd_display_enable =					(lcd_control & 0x80) ? true : false;
	window_tile_map_display_select.start =	(lcd_control & 0x40) ? 0x9C00 : 0x9800;
	window_tile_map_display_select.end =	(lcd_control & 0x40) ? 0x9FFF : 0x9BFF;
	window_display_enable =					(lcd_control & 0x20) ? true : false;
	bg_tile_data_select.start =				(lcd_control & 0x10) ? 0x8000 : 0x8800;
	bg_tile_data_select.end =				(lcd_control & 0x10) ? 0x8FFF : 0x97FF;
	bg_tile_data_select_method =			(lcd_control & 0x10) ? true : false;

	bg_tile_map_select.start =				(lcd_control & 0x08) ? 0x9C00 : 0x9800;
	bg_tile_map_select.end =				(lcd_control & 0x08) ? 0x9FFF : 0x9BFF;
	bg_tile_map_select_method =				(lcd_control & 0x08) ? true : false;
	object_size =							(lcd_control & 0x04) ? 16 : 8;
	object_display_enable =					(lcd_control & 0x02) ? true : false;
	bg_display_enable =						(lcd_control & 0x01) ? true : false;

	if (old_lcd_display_enable == true && lcd_display_enable == false)
	{
		lcd_y = 0;
		set_lcd_status_coincidence_flag(lcd_y == lcd_y_compare);
		gpu_mode = GPU_MODE_VBLANK;
		set_lcd_status_mode_flag((GPU_MODE) gpu_mode);
	}
	else if ((old_lcd_display_enable == false && lcd_display_enable == true)
		|| (old_lcd_display_enable == true && lcd_display_enable == true))
	{
		lcd_y = 0;
		set_lcd_status_coincidence_flag(lcd_y == lcd_y_compare);
	}
}

void GPU::set_lcd_status(unsigned char lcdStatus)
{
	lcd_status = lcdStatus;

	if (lcd_status & 0x40)
		enable_lcd_y_compare_interrupt = true;
	else
		enable_lcd_y_compare_interrupt = false;

	if (lcd_status & 0x20) gpu_mode = GPU_MODE_OAM;
	if (lcd_status & 0x10) gpu_mode = GPU_MODE_VBLANK;
	if (lcd_status & 0x08) gpu_mode = GPU_MODE_HBLANK;
}


void GPU::set_lcd_status_mode_flag(GPU_MODE mode)
{
	lcd_status &= 0xFC;
	if (mode != GPU_MODE_NONE)
		lcd_status |= mode;

	switch (mode)
	{
	case GPU_MODE_OAM: lcd_status |= BIT5; break;
	case GPU_MODE_VBLANK: lcd_status |= BIT4; break;
	case GPU_MODE_HBLANK: lcd_status |= BIT3; break;
	}
}

void GPU::set_lcd_status_coincidence_flag(bool flag)
{
	if (flag)
	{
		lcd_status |= 0x04;
		memory->interrupt_flag |= INTERRUPT_LCD_STATUS;
	}
	else
	{
		lcd_status &= 0xFB;
	}
}

void GPU::getTile(int tile_num, int line_num, TILE *tile)
{
	int pos = (tile_num * 16) + (line_num * 2);
	tile->b0 = vram_banks[curr_vram_bank][pos];
	tile->b1 = vram_banks[curr_vram_bank][pos + 1];
}


void GPU::renderLineTwo()
{
	// VRAM offset for which set of tiles to use
	int tile_map_offset = bg_tile_map_select.start - 0x8000;
	std::uint16_t original_tile_map_offset = tile_map_offset;

	std::uint16_t tile_offset, x_tile_offset, y_tile_offset;
	std::uint16_t actual_screen_tile_num = 0;
	x_tile_offset = (scroll_x >> 3);
	y_tile_offset = (scroll_y / 8);
	tile_offset = x_tile_offset + (y_tile_offset * 32);

	// Which line of pixels to use in the tiles
	int y = (lcd_y + scroll_y) & 0x07;

    if (lcd_y == 0)
    {
        y_roll_over = 0;
    }
    else if (y == 0)
    {
        y_roll_over++;
    }

	// Where in the tile line to start
	int x = scroll_x & 0x07;

	int map_tile_num_offset, use_tile_num;

	// Increase row of tiles to use
	tile_map_offset += (y_roll_over * 32);

	map_tile_num_offset = tile_map_offset + tile_offset;				// Get offset of tile map
	if (map_tile_num_offset >= original_tile_map_offset + 1024)			// Do a y-row rollover
		map_tile_num_offset -= 1024;
	use_tile_num = vram_banks[curr_vram_bank][map_tile_num_offset];		// Get tile number from tile map
	actual_screen_tile_num = map_tile_num_offset - original_tile_map_offset;
	logger->info("actual_screen_tile_num = {}", actual_screen_tile_num);

	Tile *tile;
	unsigned char *pixel_row = NULL;
	std::uint8_t tile_block_num;

    // Find which tile block should be used
    tile_block_num = getTileBlockNum(use_tile_num);

    // Get the tile
    tile = getTileFromBGTiles(tile_block_num, use_tile_num);

	// Get pixel row from tile
	tile->getPixelRow(y, &pixel_row);

	std::uint16_t tile_x_roll_over = actual_screen_tile_num + (SCREEN_PIXEL_W / 8);
	std::uint16_t max_tile_x_roll_over = ((y_tile_offset + y_roll_over) * 32) + 31;
	std::uint16_t max_tile_y_roll_over = bg_tile_map_select.end - bg_tile_map_select.start;

	if (lcd_y == 0 && actual_screen_tile_num == 480)
	{
		logger->info("yo");
	}

	if (bg_display_enable)
	{
		for (int i = 0; i < SCREEN_PIXEL_W; i++)
		{
			if (pixel_row != NULL)
			{
				frame[i + (lcd_y * SCREEN_PIXEL_W)] = bg_palette_color[pixel_row[x]];
			}
            else
            {
                logger->warn("pixel_row == NULL..");
            }

			x++;

			// Move on to the next tile to the right
			if (x == 8)
			{
				x = 0;
				tile_offset += 1;

				if (tile_offset > max_tile_x_roll_over)
				{
					tile_offset = ((y_tile_offset + y_roll_over) * 32);	// Do an X roll over
				}
				if (tile_offset > max_tile_y_roll_over)
				{
					tile_offset -= max_tile_y_roll_over;				// Do a Y roll over
				}


				map_tile_num_offset = tile_map_offset + tile_offset;				// Get offset of tile map
				if (map_tile_num_offset >= original_tile_map_offset + 1024)			// Do a y-row rollover
					map_tile_num_offset -= 1024;
				use_tile_num = vram_banks[curr_vram_bank][map_tile_num_offset];		// Get tile number from tile map
				actual_screen_tile_num = map_tile_num_offset - original_tile_map_offset;

                // Find which tile block should be used
                tile_block_num = getTileBlockNum(use_tile_num);

				// Get the tile
                tile = getTileFromBGTiles(tile_block_num, use_tile_num);

				// Get pixel row from tile
				tile->getPixelRow(y, &pixel_row);
			}
		}

	}
}

uint8_t GPU::getTileBlockNum(int use_tile_num)
{
    uint8_t tile_block_num = 0;

    if (bg_tile_data_select_method == 1)
    {
        if (use_tile_num < 128)
        {
            tile_block_num = 0;
        }
        else
        {
            tile_block_num = 1;
        }
    }
    else
    {
        if (use_tile_num < 128)
        {
            tile_block_num = 2;
        }
        else
        {
            tile_block_num = 1;
        }
    }
    return tile_block_num;
}

Tile * GPU::getTileFromBGTiles(uint8_t tile_block_num, int use_tile_num)
{
    Tile * tile;

    if (use_tile_num < 128)
    {
        tile = &bg_tiles[tile_block_num][use_tile_num];
    }
    else
    {
        tile = &bg_tiles[tile_block_num][use_tile_num - 128];
    }
    return tile;
}

void GPU::renderLine()
{
	// VRAM offset for which set of tiles to use
	int tile_map_offset = bg_tile_map_select.start - 0x8000;

	// Which tile to start with in the map line
	std::uint16_t tile_offset, x_tile_offset, y_tile_offset;
	x_tile_offset = (scroll_x >> 3);
	y_tile_offset = (scroll_y / 8);
	tile_offset = x_tile_offset + (y_tile_offset * 32);
	std::uint16_t original_tile_offset = tile_offset;
	std::uint16_t actual_tile_num = 0;

	// Which line of pixels to use in the tiles
	int y = (lcd_y + scroll_y) & 0x07;

	if (lcd_y == 0)
		y_roll_over = 0;
	else if (y == 0)
		y_roll_over++;

	// Where in the tile line to start
	int x = scroll_x & 0x07;

	int use_tile_num, map_tile_num_offset, map_tile_num;
	std::uint8_t color;
	TILE tile;

	// Which line of tiles to use
	tile_map_offset += (y_roll_over * 32);

	map_tile_num_offset = tile_map_offset + tile_offset;
	use_tile_num = vram_banks[curr_vram_bank][map_tile_num_offset];
	actual_tile_num = map_tile_num_offset - (bg_tile_map_select.start - 0x8000);
	
	std::uint16_t tile_x_roll_over = actual_tile_num + (SCREEN_PIXEL_W / 8);
	std::uint16_t max_tile_x_roll_over = ((y_tile_offset + y_roll_over) * 32) + 31;
	std::uint16_t max_tile_y_roll_over = bg_tile_map_select.end - bg_tile_map_select.start;

	//printf("actual_tile_num = %i, tile_x_roll_over = %i, tile_map_offset = %i, tile_offset = %i\n", actual_tile_num, tile_x_roll_over, tile_map_offset, tile_offset);
	getTile(use_tile_num, y, &tile);

	if (bg_display_enable)
	{
		for (int i = 0; i < SCREEN_PIXEL_W; i++)
		{
			actual_tile_num = map_tile_num_offset - (bg_tile_map_select.start - 0x8000);
			//printf("tile_mape_offset + tile_offset = %i, Using tile num: %i\n", tile_map_offset + tile_offset, (tile_map_offset + tile_offset) - (bg_tile_map_select.start - 0x8000));

			color = ((tile.b1 >> (6 - x)) & 0x02) | ((tile.b0 >> (7 - x)) & 0x01);
			frame[i + (lcd_y * SCREEN_PIXEL_W)] = bg_palette_color[color];

			x++;
			if (x == 8)
			{
				x = 0;
				tile_offset += 1;
				if (tile_offset > max_tile_x_roll_over)
				{
					tile_offset = ((y_tile_offset + y_roll_over) * 32);	// Do an X roll over
				}
				if (tile_offset > max_tile_y_roll_over)
				{
					tile_offset -= max_tile_y_roll_over;				// Do a Y roll over
				}
				map_tile_num_offset = tile_map_offset + tile_offset;
				use_tile_num = vram_banks[curr_vram_bank][map_tile_num_offset & (bg_tile_map_select.end - 0x8000)];
				actual_tile_num = map_tile_num_offset - (bg_tile_map_select.start - 0x8000);
				//printf("actual_tile_num = %i, tile_map_offset = %i, tile_offset = %i\n", actual_tile_num, tile_map_offset, tile_offset);
				getTile(use_tile_num, y, &tile);
			}
		}
	}

	//printf("y = %i, lcd_y = %i\n", y, lcd_y);

	if (object_display_enable)
	{
		std::uint8_t curr_sprite;
		std::uint8_t sprite_y, sprite_x, sprite_tile_num, byte3;
		bool below_background, sprite_y_flip, sprite_x_flip, sprite_palette_num;

		for (int i = 0; i < object_attribute_memory.size(); i += 4)
		{
			curr_sprite = i % 4;
			sprite_y		= object_attribute_memory[i] - 16;
			sprite_x		= object_attribute_memory[i + 1] - 8;
			sprite_tile_num = object_attribute_memory[i + 2];
			byte3			= object_attribute_memory[i + 3];

			below_background	= byte3 & 0x80;
			sprite_y_flip		= byte3 & 0x40;
			sprite_x_flip		= byte3 & 0x20;
			sprite_palette_num	= byte3 & 0x10;

			// Check to see if sprite is rendered on current line
			if (sprite_y <= scroll_y && (sprite_y + 8) > scroll_y)
			{

				// Get tile to use
				getTile(sprite_tile_num, y, &tile);

				// Find which row of tile to  render

				
				// Draw row of pixels
				for (int x = 0; x < 8; x++)
				{
					if ((sprite_x + x) >= 0 && (sprite_x + x) < SCREEN_PIXEL_W && !below_background)
					{
						color = ((tile.b1 >> (6 - x)) & 0x02) | ((tile.b0 >> (7 - x)) & 0x01);

						if (sprite_palette_num == 0)
						{
							frame[x + sprite_x + (lcd_y * SCREEN_PIXEL_W)] = object_palette0_color[color];
						}
						else
						{
							frame[x + sprite_x + (lcd_y * SCREEN_PIXEL_W)] = object_palette1_color[color];
						}
					}


				}
			}


		}
	}

}


void GPU::display()
{
#ifdef SDL_DRAW
#ifdef DEBUG
	// Render partial screen with debug info
	//SDL_Rect game_screen_rect = { 0, 0, SCREEN_PIXEL_W * 2, SCREEN_PIXEL_H * 2 };
	SDL_Rect game_screen_rect = { 0, 0, SCREEN_PIXEL_W * 4, SCREEN_PIXEL_H * 4 };
	SDL_UpdateTexture(game_screen, NULL, frame, SCREEN_PIXEL_W * sizeof(SDL_Color));
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, game_screen, NULL, &game_screen_rect);
	SDL_RenderPresent(renderer);
#else
	// Render full screen
	SDL_UpdateTexture(game_screen, NULL, frame, SCREEN_PIXEL_W * sizeof(unsigned char) * 4);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, game_screen, NULL, NULL);
	SDL_RenderPresent(renderer);
#endif
#endif
}

void GPU::run()
{
	if (lcd_display_enable == false) return;

	ticks += cpu->ticks - last_ticks;
	last_ticks = cpu->ticks;

	switch (gpu_mode)
	{
	case GPU_MODE_HBLANK:

		if (ticks >= 204)
		{
			if (lcd_y == 0)
				logger->info("Start Frame");

			lcd_y++;

			// Check if frame rendering has completed, start VBLANK interrupt
			if (lcd_y == 144)
			{
				//if (memory->interrupt_enable & INTERRUPT_VBLANK)
				//	memory->interrupt_flag |= INTERRUPT_VBLANK;
				memory->interrupt_flag |= INTERRUPT_VBLANK;
				gpu_mode = GPU_MODE_VBLANK;
			}
			else
			{
				gpu_mode = GPU_MODE_OAM;
			}

			ticks = 0;
		}
		break;


	case GPU_MODE_VBLANK:

		if (ticks >= 456)
		{
			lcd_y++;

			if (lcd_y > 153)
			{
                frame_is_ready = true;
				display();
				lcd_y = 0;
				gpu_mode = GPU_MODE_OAM;

				//if (SDL_PollEvent(&e) != 0 && e.type == e.key.type && e.key.keysym.scancode != SDL_SCANCODE_UNKNOWN)
				//{
				//	memory->joypad->check_keyboard_input(&e);
				//}
				logger->info("End Frame");
			}

			ticks = 0;
		}
		break;


	case GPU_MODE_OAM:

		if (ticks >= 80)
		{
			gpu_mode = GPU_MODE_VRAM;
			ticks = 0;
		}
		break;


	case GPU_MODE_VRAM:

		if (ticks >= 172)
		{
			//renderLine();
			renderLineTwo();
			gpu_mode = GPU_MODE_HBLANK;
			ticks = 0;
		}
		break;
	}

	if (lcd_y == lcd_y_compare)
		set_lcd_status_coincidence_flag(true);
	else
		set_lcd_status_coincidence_flag(false);

	if (gpu_mode != GPU_MODE_NONE)
		set_lcd_status_mode_flag((GPU_MODE)gpu_mode);
}


void GPU::printFrame()
{
	std::string s;

	for (int i = 0; i < SCREEN_PIXEL_H; i++)
	{
		s = "";

		for (int j = 0; j < SCREEN_PIXEL_W; j++)
		{
			switch (frame[j + (i * SCREEN_PIXEL_H)].r)
			{
			case 255: s += "0"; break;
			case 192: s += "1"; break;
			case 96: s += "2"; break;
			case 0: s += "3"; break;
			}
		}
		s += "\n";
	}
	logger->debug("{}", s.c_str());
}


void GPU::updateTile(std::uint16_t pos, std::uint8_t val, std::uint8_t tile_block_num)
{
	Tile *tile;
	std::uint16_t tile_num;
	std::uint16_t byte_pos;
	std::uint16_t offset;

	offset = 0x8000 + (0x800 * tile_block_num);

	if (pos - offset >= 0)
	{
		byte_pos = pos - offset;
		tile_num = std::floor(byte_pos / NUM_BYTES_PER_TILE);
		tile = &bg_tiles[tile_block_num][tile_num];
		tile->updateRawData(byte_pos % 16, val);
        bg_tiles_updated = true;
	}
}

const SDL_Color * GPU::getFrame()
{
    return frame;
}

const std::vector<std::vector<Tile>> & GPU::getBGTiles()
{
    bg_tiles_updated = false;
    return bg_tiles;
}