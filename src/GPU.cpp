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
    last_ticks = 0;
    lcd_display_enable = false;
    lcd_status_interrupt_signal = false;

	lcd_control = NULL;

	vram_banks.resize(num_vram_banks, std::vector<unsigned char>(VRAM_SIZE, 0));
	object_attribute_memory.resize(OAM_SIZE);
	bg_tiles.resize(NUM_BG_TILE_BLOCKS, std::vector<Tile>(NUM_BG_TILES_PER_BLOCK));

	background_palette_data.resize(PALETTE_DATA_SIZE * PALETTE_DATA_SIZE);
	sprite_palette_data.resize(PALETTE_DATA_SIZE);

    bg_frame.resize(TOTAL_SCREEN_PIXEL_W * TOTAL_SCREEN_PIXEL_H);

	gpu_mode = GPU_MODE_OAM;

#ifdef SDL_DRAW
	renderer = render;
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	game_screen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, SCREEN_PIXEL_W, SCREEN_PIXEL_H);
#endif

	background_palette_index = 0;
	sprite_palette_index = 0;
    scroll_x = 0;
    scroll_y = 0;
    lcd_y = 0;
    lcd_y_compare = 0;
    y_roll_over = 0;
    bg_display_enable = 0;
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
    is_color_gb = true;
	num_vram_banks = 2;
	vram_banks.resize(num_vram_banks, std::vector<unsigned char>(VRAM_SIZE, 0));
}

std::uint8_t GPU::readByte(std::uint16_t pos)
{
	switch (pos & 0xF000)
	{
	case 0x8000:
	case 0x9000:

        // Block writing to VRAM during Modes VRAM
        //if (lcd_status & 0x03 == GPU_MODE_VRAM)
        //{
        //    logger->info("CPU cannot access OAM when GPU is using OAM, blocking CPU write");
        //    return 0xFF;
        //}

		return vram_banks[curr_vram_bank][pos - 0x8000];

	case 0xF000:

		if (pos < 0xFF00)
		{
            // Block writing to OAM during Modes VRAM and OAM
            //if (lcd_status & 0x03 == GPU_MODE_OAM ||
            //    lcd_status & 0x03 == GPU_MODE_VRAM)
            //{
            //    logger->info("CPU cannot access OAM when GPU is using OAM, blocking CPU write");
            //    return 0xFF;
            //}

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
			case 0xFF4F:	return curr_vram_bank | 0xFE;   // Only bit 0 matters, rest will be 1s
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

        // Block writing to VRAM during Modes VRAM
        //if (lcd_status & 0x03 == GPU_MODE_VRAM)
        //{
        //    logger->info("CPU cannot access OAM when GPU is using OAM, blocking CPU write");
        //    return;
        //}

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
            // Block writing to VRAM during Modes VRAM and OAM
            //if (lcd_status & 0x03 == GPU_MODE_OAM ||
            //    lcd_status & 0x03 == GPU_MODE_VRAM)
            //{
            //    logger->info("CPU cannot access OAM when GPU is using OAM, blocking CPU write");
            //    return;
            //}

			// 0xFE00 - 0xFE9F : Sprite RAM (OAM)
			object_attribute_memory[pos - 0xFE00] = val;
			break;
		}
		else if (pos < 0xFF6C)
		{
			// LCD Stuff, VRAM bank selector

            // CGB only
            if (memory->cartridgeReader->getColorGBFlag())
            {
                switch (pos)
                {
                case 0xFF4F:	curr_vram_bank = (val & 0x01); return;
                case 0xFF51:	hdma1 = val; return;
                case 0xFF52:	hdma2 = val; return;
                case 0xFF53:	hdma3 = val; return;
                case 0xFF54:	hdma4 = val; return;
                case 0xFF55:	hdma5 = val; return;

                    // Background Palette Index
                case 0xFF68:
                    background_palette_index = val & 0x3F;
                    auto_increment_background_palette_index = val & 0x80;
                    return;
                    // Background Palette Data
                case 0xFF69:
                    background_palette_data[background_palette_index] = val;
                    updateBackgroundPalette(val);
                    if (auto_increment_background_palette_index)
                    {
                        background_palette_index++;
                    }
                    background_palette_index &= 0x3F;
                    return;
                    // Sprite Palette Index
                case 0xFF6A:
                    sprite_palette_index = val & 0x07;
                    auto_increment_sprite_palette_index = val & 0x80;
                    return;
                    // Sprite Palette Data

                case 0xFF6B:
                    sprite_palette_data[sprite_palette_index] = val;
                    updateSpritePalette(val);
                    if (auto_increment_sprite_palette_index)
                    {
                        sprite_palette_index++;
                    }
                    sprite_palette_index &= 0x07;
                    return;
                }
            }

			switch (pos)
			{
			case 0xFF40:	set_lcd_control(val); break;
            case 0xFF41:	set_lcd_status(val); break;
			case 0xFF42:	scroll_y = val; break;
			case 0xFF43:	scroll_x = val; break;
			case 0xFF44:	if (lcd_display_enable == false) lcd_y = 0; break;				// Read only - Writing to this register resets the counter
			case 0xFF45:	lcd_y_compare = val; set_lcd_status_coincidence_flag(lcd_y == lcd_y_compare); break;
			case 0xFF46:	memory->do_oam_dma_transfer(val); break;
			case 0xFF47:	bg_palette = val;       set_color_palette(bg_palette_color, val); break;
			case 0xFF48:	object_pallete0 = val;  set_color_palette(object_palette0_color, val, true); break;
			case 0xFF49:	object_pallete1 = val;  set_color_palette(object_palette1_color, val, true); break;
			case 0xFF4A:	window_y_pos = val; break;
			case 0xFF4B:	window_x_pos = val; break;

			default:
				logger->warn("GPU::setByte() doesn't handle address: 0x{0:x}", pos);
			}
		}
		break;
	default:
		logger->warn("GPU::setByte() doesn't handle address: 0x{0:x}", pos);
	}
}


void GPU::set_color_palette(SDL_Color *palette, std::uint8_t val, bool zero_is_transparant)
{
	unsigned char color_val = 0;
    unsigned char alpha = 255;

	for (int i = 0; i < 4; i++)
	{
        if (i == 0 && zero_is_transparant)
        {   // First two bits (i == 0) for object_palettes are transparent, e.g. alpha = max
            alpha = 0;
        }
        else
        {
            alpha = 255;
        }

		switch ((val >> (i * 2)) & 3)
		{
		case 0: color_val = 255; break;
		case 1: color_val = 192; break;
		case 2: color_val = 96; break;
		case 3: color_val = 0; break;
		}
		palette[i] = { color_val, color_val, color_val, alpha };
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
        set_lcd_status_mode_flag(GPU_MODE_VBLANK);
        ticks = 0;
	}
	else if ((old_lcd_display_enable == false && lcd_display_enable == true)
		|| (old_lcd_display_enable == true && lcd_display_enable == true))
	{
		lcd_y = 0;
		set_lcd_status_coincidence_flag(lcd_y == lcd_y_compare);
        ticks = 0;
	}
}

void GPU::set_lcd_status(unsigned char lcdStatus)
{
	lcd_status = lcdStatus & 0xF8;  // First 3 bits are read-only

    if (lcd_status & BIT6)
    {
        enable_lcd_y_compare_interrupt = true;
    }
    else
    {
        enable_lcd_y_compare_interrupt = false;
    }

	if (lcd_status & 0x20) gpu_mode = GPU_MODE_OAM;
	if (lcd_status & 0x10) gpu_mode = GPU_MODE_VBLANK;
	if (lcd_status & 0x08) gpu_mode = GPU_MODE_HBLANK;
}


void GPU::set_lcd_status_mode_flag(GPU_MODE mode)
{
    int prev_gpu_mode = gpu_mode;
    gpu_mode = mode;

    // Clear bits 0 and 1
    lcd_status &= 0xFC;

    // Set bits 0 and 1
    if (mode != GPU_MODE_NONE)
    {
        lcd_status |= mode;
    }

    // Check if an interrupt should be fired off
    if (prev_gpu_mode != gpu_mode)
    {
        if ((mode == GPU_MODE_HBLANK && lcd_status & BIT3) ||
            (mode == GPU_MODE_VBLANK && lcd_status & BIT4) ||
            (mode == GPU_MODE_OAM && lcd_status & BIT5))
        {
            lcd_status_interrupt_signal = 0;
            memory->interrupt_flag |= INTERRUPT_LCD_STATUS;
        }
    }
}

void GPU::set_lcd_status_coincidence_flag(bool flag)
{
    // Update lcd_status' lcd_y == lcd_y_compare bit
    if (flag)
    {   // lcd_y == lcd_y_compare
        lcd_status |= BIT2;

        if (lcd_status & BIT6)
        {
            memory->interrupt_flag |= INTERRUPT_LCD_STATUS;
        }
    }
    else
    {
        lcd_status &= 0xFB;
    }

    if (lcd_display_enable == false)
    {
        lcd_status_interrupt_signal = 0;
    }

    // Check if an interrupt flag should be enabled
	//if (((lcd_status & BIT2) && (lcd_status & BIT6))
 //       || (gpu_mode == GPU_MODE_HBLANK && (lcd_status & BIT3))
 //       || (gpu_mode == GPU_MODE_VBLANK && ((lcd_status & BIT4) || (lcd_status & BIT5)))
 //       || (gpu_mode == GPU_MODE_OAM    && (lcd_status & BIT5)))
	//{
 //       // Only trigger interrupt when going from low to high
 //       if (lcd_status_interrupt_signal == 0)
 //       {
 //           lcd_status_interrupt_signal = 1;
 //           memory->interrupt_flag |= INTERRUPT_LCD_STATUS;
 //       }
	//}
}

void GPU::getTile(int tile_num, int line_num, TILE *tile)
{
	int pos = (tile_num * 16) + (line_num * 2);
	tile->b0 = vram_banks[curr_vram_bank][pos];
	tile->b1 = vram_banks[curr_vram_bank][pos + 1];
}

// Draws the 256x256 pixel Background to bg_frame[]
void GPU::renderFullBackgroundMap()
{
    int tile_map_offset = bg_tile_map_select.start - 0x8000;
    int tile_offset = 0;
    int tile_offset_save = 0;
    int map_tile_num_offset = 0;
    int use_tile_num = 0;

    Tile *tile;
    std::uint8_t tile_block_num;
    uint16_t row_pixel_offset;

    for (uint16_t tile_map_pos = 0; tile_map_pos < bg_tile_map_select.end - bg_tile_map_select.start + 1; tile_map_pos++)
    {
        map_tile_num_offset = tile_map_offset + tile_map_pos;

        // Get tile number from tile map
        use_tile_num = vram_banks[curr_vram_bank][map_tile_num_offset];

        // Find which tile block should be used
        tile_block_num = getTileBlockNum(use_tile_num);

        // Get the tile
        tile = getTileFromBGTiles(tile_block_num, use_tile_num);

        const std::vector<uint8_t> tile_data = tile->getRawPixelData();

        row_pixel_offset = ((tile_map_pos / 32) * 8 * 256);

        for (uint8_t row = 0; row < 8; row++)
        {
            uint16_t use_row = (row * 256) + row_pixel_offset; // (0..65535)

            for (uint8_t col = 0; col < 8; col++)
            {
                uint8_t use_col = col + ((tile_map_pos * 8) & 255); // (0..255)

                // Set pixel in frame
                bg_frame[use_col + use_row] = bg_palette_color[tile_data[col + (row * 8)]];
            }
        }
    }

    drawShownBackgroundArea();
}

// Draws a rectangle using SCX and SCY to show the area 
// that is displayed of the Background bg_frame
void GPU::drawShownBackgroundArea()
{
    uint8_t start_x = scroll_x;
    uint8_t start_y = scroll_y;

    SDL_Color black = { 0, 0, 0, 0 };

    uint16_t top_line_offset, bottom_line_offset, left_line_offset, right_line_offset;

    // Draw horizontal lines
    for (uint8_t i = 0; i < SCREEN_PIXEL_W; i++)
    {   // Calculate line offsets
        top_line_offset     = start_x + i + (start_y * 256);
        bottom_line_offset  = start_x + i + ((start_y + SCREEN_PIXEL_H) * 256);

        // Check if x + i rolled over
        if (start_x + i >= 256)
        {   // Make sure the horizontal line rollover will be on the same horizontal y-offset
            top_line_offset    -= 256;
            bottom_line_offset -= 256;
        }

        // Draw lines
        bg_frame[top_line_offset]       = black;
        bg_frame[bottom_line_offset]    = black;
    }

    // Draw vertical lines
    for (uint8_t i = 0; i < SCREEN_PIXEL_H; i++)
    {   // Calculate line offsets
        left_line_offset = start_x + ((start_y + i) * 256);
        right_line_offset = start_x + SCREEN_PIXEL_W + ((start_y + i) * 256);

        // Draw lines
        bg_frame[left_line_offset] = black;
        bg_frame[right_line_offset] = black;
    }
}

void GPU::drawBackgroundLine()
{
    Tile * tile;

    // Calculate which row of the Tile we're in (0..7)
    uint8_t curr_tile_row = (lcd_y + scroll_y) & 0x07;

    // Get VRAM offset for which set of tiles to use
    uint16_t tile_map_vram_offset = bg_tile_map_select.start - 0x8000;

    uint8_t use_pixel_x = scroll_x;
    uint8_t use_pixel_y = scroll_y + lcd_y;     // Will rollover naturally due to uint8 (0..255)

    uint16_t frame_y = lcd_y * SCREEN_PIXEL_W;

    // Draw scanline
    for (uint8_t frame_x = 0; frame_x < SCREEN_PIXEL_W; frame_x++)
    {
        // Get tile offset in tile_map
        uint16_t tile_map_offset = getTileMapNumber(use_pixel_x, use_pixel_y);

        // Get tile number from tile map
        uint16_t use_tile_num = vram_banks[curr_vram_bank][tile_map_vram_offset + tile_map_offset];

        // Find which tile memory block should be used
        uint8_t tile_block_num = getTileBlockNum(use_tile_num);

        // Get the tile
        tile = getTileFromBGTiles(tile_block_num, use_tile_num);

        // Calculate which col of the Tile we're in (0..7)
        uint8_t curr_tile_col = (scroll_x + frame_x) & 0x07;

        // Get pixel
        uint8_t pixel = tile->getPixel(curr_tile_row, curr_tile_col);

        // Draw pixel
        frame[frame_x + frame_y] = bg_palette_color[pixel];
        //frame[frame_x + frame_y] = background_palettes[background_palette_index / 8].getColor(pixel);

        // Will rollover naturally due to uint8 (0..255)
        use_pixel_x++;
    }
}

void GPU::drawWindowLine()
{
    Tile * tile;

    // Calculate which row of the Tile we're in (0..7)
    uint8_t curr_tile_row = (lcd_y + window_y_pos) & 0x07;

    // Get VRAM offset for which set of tiles to use
    uint16_t tile_map_vram_offset = window_tile_map_display_select.start - 0x8000;

    uint8_t use_pixel_x = window_x_pos - 7;
    uint8_t use_pixel_y = window_y_pos + lcd_y;     // Will rollover naturally due to uint8 (0..255)

    uint16_t frame_y = lcd_y * SCREEN_PIXEL_W;

    if (use_pixel_x >= SCREEN_PIXEL_W || window_y_pos >= SCREEN_PIXEL_H)
    {
        return;
    }

    if (lcd_y < window_y_pos)
    {
        return;
    }

    // Draw scanline
    for (uint8_t frame_x = 0; frame_x < SCREEN_PIXEL_W; frame_x++)
    {
        // Get tile in tile_map
        uint16_t tile_map_offset = getTileMapNumber(use_pixel_x, use_pixel_y);

        // Get tile number from tile map
        uint16_t use_tile_num = vram_banks[curr_vram_bank][tile_map_vram_offset + tile_map_offset];

        // Find which tile memory block should be used
        uint8_t tile_block_num = getTileBlockNum(use_tile_num);

        // Get the tile
        tile = getTileFromBGTiles(tile_block_num, use_tile_num);

        // Calculate which col of the Tile we're in (0..7)
        uint8_t curr_tile_col = (scroll_x + frame_x) & 0x07;

        // Get pixel
        uint8_t pixel = tile->getPixel(curr_tile_row, curr_tile_col);

        // Draw pixel
        frame[frame_x + frame_y] = bg_palette_color[pixel];

        // Will rollover naturally due to uint8 (0..255)
        use_pixel_x++;
    }
}

void GPU::drawOAMLine()
{
    Tile * tile;
    std::uint8_t curr_sprite;
    std::uint8_t sprite_y, sprite_x, sprite_tile_num, byte3;
    uint8_t pixel_x, pixel_y;
    uint8_t tile_vram_bank_num;
    uint8_t sprite_palette_num_cgb;
    bool object_behind_bg, sprite_y_flip, sprite_x_flip, sprite_palette_num;

    pixel_x = pixel_y = 0;

    for (int i = 0; i < object_attribute_memory.size(); i += 4)
    {
        // Parse current sprite's 4 bytes of data
        curr_sprite     = i / 4;
        sprite_y        = object_attribute_memory[i] - 16;
        sprite_x        = object_attribute_memory[i + 1] - 8;
        sprite_tile_num = object_attribute_memory[i + 2];
        byte3           = object_attribute_memory[i + 3];

        object_behind_bg    = byte3 & 0x80;
        sprite_y_flip       = byte3 & 0x40;
        sprite_x_flip       = byte3 & 0x20;
        sprite_palette_num  = byte3 & 0x10; // Non CGB Mode only

        // CGB Mode only
        tile_vram_bank_num      = (byte3 & 0x08) >> 3;
        sprite_palette_num_cgb  = byte3 & 0x07;

        // Check to see if sprite is rendered on current line (Y position)
        if (sprite_y <= lcd_y && (sprite_y + object_size) > lcd_y)
        {
            // Check for case of object_size = 16, ie sprite size is 8x16
            // Then check if we should be using the next sprite 8x8 sprite to draw
            if (object_size == 16)
            {
                uint8_t curr_sprite_y = (sprite_y + object_size) - lcd_y;
                if (!sprite_y_flip && curr_sprite_y <= 8)
                {   // Get bottom 8x8 sprite
                    sprite_tile_num++;
                }
                else if (sprite_y_flip && curr_sprite_y >= 8)
                {   // Get bottom 8x8 sprite
                    sprite_tile_num++;
                }
            }

            // Find which tile block should be used
            uint8_t tile_block_num = getSpriteTileBlockNum(sprite_tile_num);

            // Get the tile
            tile = getTileFromBGTiles(tile_block_num, sprite_tile_num);

            // Draw row of pixels
            for (uint8_t x = 0; x < 8; x++)
            {
                // Check to make sure sprite X position isn't out of bounds
                if ((sprite_x + x + 1) > SCREEN_PIXEL_W)
                {
                    continue;
                }

                // Check if pixel should be drawn due to object_behind_bg flag
                if (object_behind_bg)
                {
                    // Get current pixel in frame
                    auto curr_frame_pixel = frame[x + sprite_x + (lcd_y * SCREEN_PIXEL_W)];

                    if (bg_display_enable == false && is_color_gb)
                    {   // Let sprite be drawn on top of background and window

                    }
                    else if (SDLColorsAreEqual(curr_frame_pixel, bg_palette_color[0]) == false)
                    {   // If curr_frame_pixel isn't background color 0, don't draw object's current pixel
                        continue;
                    }
                }

                // Find out which col of the sprite to use for this pixel
                pixel_x = x;
                // Find out which row of the sprite to use for this line
                pixel_y = lcd_y - sprite_y;

                // Check if sprite needs to be drawn flipped
                if (sprite_y_flip)
                {   // Vertically mirrored
                    pixel_y = object_size - 1 - pixel_y;
                }

                if (sprite_x_flip)
                {   // Horizontally mirrored
                    pixel_x = 7 - pixel_x;
                }

                if (object_size == 16)
                {
                    if (pixel_y >= 8)
                    {
                        pixel_y -= 8;
                    }
                }

                // Get color for current pixel in object
                const uint8_t pixel_color = tile->getPixel(pixel_y, pixel_x);

                if (pixel_color == 0)
                {
                    continue;   // Color 0 == transparent == don't display
                }

                uint8_t use_x = x + sprite_x;

                // Draw sprite pixel to frame
                //if (is_color_gb)
                //{   // Draw sprite in color
                //    frame[use_x + (lcd_y * SCREEN_PIXEL_W)] = sprite_palettes[sprite_palette_num_cgb].getColor(pixel_color);
                //}
                //else
                {   // Draw sprite in gray scale
                    if (sprite_palette_num == 0)
                    {
                        frame[use_x + (lcd_y * SCREEN_PIXEL_W)] = object_palette0_color[pixel_color];
                    }
                    else
                    {
                        frame[use_x + (lcd_y * SCREEN_PIXEL_W)] = object_palette1_color[pixel_color];
                    }
                }

            } // end for(x)
        } // end if(y)
    } //end for(sprite)
}

void GPU::renderLine()
{
    if (bg_display_enable)
    {
        drawBackgroundLine();
    }

    if (window_display_enable)
    {
        drawWindowLine();
    }

    if (object_display_enable)
    {
        drawOAMLine();
    }
}

uint16_t GPU::getTileMapNumber(uint8_t pixel_x, uint8_t pixel_y)
{
    // Calculate Tile row
    uint8_t tile_row = pixel_y / 8;

    // Calculate Tile column
    uint8_t tile_col = pixel_x / 8;

    // Get actual tile number for wanted row
    uint16_t tile_row_num = tile_row * 32;

    // Get actual tile number offset (include col offset)
    uint16_t tile_num = tile_row_num + tile_col;

    return tile_num;
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

uint8_t GPU::getSpriteTileBlockNum(int use_tile_num)
{
    return use_tile_num / 128;
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
    if (lcd_display_enable == false)
    {
        last_ticks = cpu->ticks;

        if (lcd_status & 0x03 != GPU_MODE_HBLANK)
        {
            set_lcd_status_mode_flag(GPU_MODE_HBLANK);
        }
        return;
    }

	ticks += cpu->ticks - last_ticks;
	last_ticks = cpu->ticks;

    switch (lcd_status & 0x03)
	{
	case GPU_MODE_HBLANK:

		if (ticks >= 204)
		{
            if (lcd_y == 0)
            {
                logger->debug("Start Frame");
            }

			lcd_y++;

			// Check if frame rendering has completed, start VBLANK interrupt
            if (lcd_y == 144)
            {
                renderFullBackgroundMap();
                memory->interrupt_flag |= INTERRUPT_VBLANK;
                set_lcd_status_mode_flag(GPU_MODE_VBLANK);
            }
            else
            {
                set_lcd_status_mode_flag(GPU_MODE_OAM);
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
                set_lcd_status_mode_flag(GPU_MODE_OAM);
				logger->debug("End Frame");
			}

			ticks = 0;
		}
		break;


	case GPU_MODE_OAM:

		if (ticks >= 80)
		{
            set_lcd_status_mode_flag(GPU_MODE_VRAM);
			ticks = 0;
		}
		break;


	case GPU_MODE_VRAM:

		if (ticks >= 172)
		{
            renderLine();
            set_lcd_status_mode_flag(GPU_MODE_HBLANK);
			ticks = 0;
		}
		break;
	}

    set_lcd_status_coincidence_flag(lcd_y == lcd_y_compare);
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

std::vector<std::vector<Tile>> GPU::getBGTiles()
{
    bg_tiles_updated = false;
    return bg_tiles;
}

bool GPU::SDLColorsAreEqual(const SDL_Color & a, const SDL_Color & b)
{
    if (a.r == b.r &&
        a.b == b.b &&
        a.g == b.g &&
        a.a == b.a)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void GPU::updateBackgroundPalette(uint8_t val)
{
    // Get Color Palette object
    ColorPalette & colorPalette = background_palettes[background_palette_index / 8];

    // Update color palette
    colorPalette.updateRawByte(background_palette_index % 8, val);
}

void GPU::updateSpritePalette(uint8_t val)
{
    // Get Color Palette object
    ColorPalette & colorPalette = sprite_palettes[sprite_palette_index / 8];

    // Update color palette
    colorPalette.updateRawByte(sprite_palette_index % 8, val);
}