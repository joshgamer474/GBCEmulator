#ifdef _WIN32
#include "stdafx.h"
#endif // _WIN32

#include "GPU.h"
#include "Joypad.h"
#include "Memory.h"
#include "Debug.h"
#include "Tile.h"
#include "CartridgeReader.h"

GPU::GPU(std::shared_ptr<spdlog::logger> _logger)
    : logger(_logger)
    , curr_opt_gb_palette(CGBPaletteCombo::CUSTOM)  // Default to DMG-custom palette
{
	is_color_gb = false;
	num_vram_banks = 1;
	curr_vram_bank = 0;
	ticks_accumulated = 0;
    lcd_display_enable = false;
    lcd_status_interrupt_signal = false;
    wait_frame_to_render_window = false;
    render_full_frame = false;

	lcd_control = 0;

	vram_banks.resize(num_vram_banks, std::vector<unsigned char>(VRAM_SIZE, 0));
	object_attribute_memory.resize(OAM_SIZE);
    bg_tiles.resize(num_vram_banks);
    for (int i = 0; i < num_vram_banks; i++)
    {
        bg_tiles[i].resize(NUM_BG_TILE_BLOCKS, std::vector<Tile>(NUM_BG_TILES_PER_BLOCK));
    }

    cgb_bg_to_oam_priority_array.fill(0);

	gpu_mode = GPU_MODE_OAM;

	cgb_background_palette_index = 0;
	cgb_sprite_palette_index = 0;
    scroll_x = 0;
    scroll_y = 0;
    lcd_y = 0;
    lcd_y_compare = 0;
    y_roll_over = 0;
    bg_display_enable = 0;
	bg_tile_data_select_method = false;
	bg_tile_map_select_method = false;
	cgb_auto_increment_background_palette_index = false;
	cgb_auto_increment_sprite_palette_index = false;
    is_cgb_tile_palette_updated = false;
    is_tile_palette_updated     = false;

    cgb_dma_in_progress = false;
    cgb_dma_hblank_in_progress = false;
    cgb_dma_transfer_bytes_left = 0;

    frame_is_ready = false;
    bg_tiles_updated = false;

    objects_pos_to_use.resize(OAM_NUM_SPRITES);
}


GPU::~GPU()
{
    memory.reset();
    logger.reset();
}

GPU& GPU::operator=(const GPU& rhs)
{   // Copy from rhs
    is_color_gb         = rhs.is_color_gb;
    num_vram_banks      = rhs.num_vram_banks;
    curr_vram_bank      = rhs.curr_vram_bank;
    ticks_accumulated   = rhs.ticks_accumulated;
    lcd_display_enable  = rhs.lcd_display_enable;
    bg_display_enable   = rhs.bg_display_enable;
    window_display_enable = rhs.window_display_enable;
    object_display_enable = rhs.object_display_enable;
    lcd_status_interrupt_signal = rhs.lcd_status_interrupt_signal;
    wait_frame_to_render_window = rhs.wait_frame_to_render_window;
    render_full_frame   = rhs.render_full_frame;
    lcd_control         = rhs.lcd_control;
    vram_banks          = rhs.vram_banks;
    object_attribute_memory = rhs.object_attribute_memory;
    bg_tiles            = rhs.bg_tiles;
    gpu_mode            = rhs.gpu_mode;
    cgb_background_palette_index = rhs.cgb_background_palette_index;
    cgb_sprite_palette_index = rhs.cgb_sprite_palette_index;
    scroll_x            = rhs.scroll_x;
    scroll_y            = rhs.scroll_y;
    lcd_y               = rhs.lcd_y;
    lcd_y_compare       = rhs.lcd_y_compare;
    y_roll_over         = rhs.y_roll_over;
    bg_tile_data_select_method = rhs.bg_tile_data_select_method;
    bg_tile_map_select_method = rhs.bg_tile_map_select_method;
    window_tile_map_display_select = rhs.window_tile_map_display_select;
    bg_tile_data_select = rhs.bg_tile_data_select;
    bg_tile_map_select = rhs.bg_tile_map_select;
    is_cgb_tile_palette_updated = rhs.is_cgb_tile_palette_updated;
    is_tile_palette_updated = rhs.is_tile_palette_updated;

    cgb_auto_increment_background_palette_index = rhs.cgb_auto_increment_background_palette_index;
    cgb_auto_increment_sprite_palette_index = rhs.cgb_auto_increment_sprite_palette_index;
    cgb_background_palettes         = rhs.cgb_background_palettes;
    cgb_sprite_palettes             = rhs.cgb_sprite_palettes;
    cgb_background_palette_data     = rhs.cgb_background_palette_data;
    cgb_sprite_palette_data         = rhs.cgb_sprite_palette_data;
    cgb_bg_to_oam_priority_array    = rhs.cgb_bg_to_oam_priority_array;
    cgb_bg_scanline_color_palettes  = rhs.cgb_bg_scanline_color_palettes;

    hdma1 = rhs.hdma1;
    hdma2 = rhs.hdma2;
    hdma3 = rhs.hdma3;
    hdma4 = rhs.hdma4;
    hdma5 = rhs.hdma5;
    cgb_dma_in_progress         = rhs.cgb_dma_in_progress;
    cgb_dma_hblank_in_progress  = rhs.cgb_dma_hblank_in_progress;
    cgb_dma_transfer_bytes_left = rhs.cgb_dma_transfer_bytes_left;

    object_size = rhs.object_size;

    enable_lcd_y_compare_interrupt = rhs.enable_lcd_y_compare_interrupt;

    frame_is_ready      = rhs.frame_is_ready;
    bg_tiles_updated    = rhs.bg_tiles_updated;
    objects_pos_to_use  = rhs.objects_pos_to_use;

    memcpy(frame, rhs.frame, SCREEN_PIXEL_W * SCREEN_PIXEL_H * sizeof(SDL_Color));
    memcpy(curr_frame, rhs.curr_frame, SCREEN_PIXEL_W * SCREEN_PIXEL_H * sizeof(SDL_Color));

    memcpy(bg_palette_color, rhs.bg_palette_color, PALETTE_DATA_SIZE * sizeof(SDL_Color));
    memcpy(object_palette0_color, rhs.object_palette0_color, PALETTE_DATA_SIZE * sizeof(SDL_Color));
    memcpy(object_palette1_color, rhs.object_palette1_color, PALETTE_DATA_SIZE * sizeof(SDL_Color));

    return *this;
}

void GPU::init_color_gb()
{
    is_color_gb = true;
	num_vram_banks = 2;
	vram_banks.resize(num_vram_banks, std::vector<unsigned char>(VRAM_SIZE, 0));

    bg_tiles.resize(num_vram_banks);
    for (int i = 0; i < num_vram_banks; i++)
    {
        bg_tiles[i].resize(NUM_BG_TILE_BLOCKS, std::vector<Tile>(NUM_BG_TILES_PER_BLOCK));
    }

    // Initialize OAM sprite order for CGB
    for (int i = 0; i < OAM_NUM_SPRITES; i++)
    {
        objects_pos_to_use[i] = (OAM_NUM_SPRITES - i - 1) * 4;
    }

    // Reset DMG-only variable
    curr_opt_gb_palette = CGBPaletteCombo::NONE;
}

std::uint8_t GPU::readByte(const uint16_t& pos, const bool limit_access) const
{
	switch (pos & 0xF000)
	{
	case 0x8000:
	case 0x9000:

        // Block writing to VRAM during VRAM Mode
        if (limit_access && 
           (lcd_status & 0x03) == GPU_MODE_VRAM)
        {
            logger->info("CPU cannot access OAM when GPU is using OAM, blocking CPU write");
            return 0xFF;
        }

		return vram_banks[curr_vram_bank % num_vram_banks][pos - 0x8000];

	case 0xF000:

		if (pos < 0xFF00)
		{
            // Block writing to OAM during Modes VRAM and OAM
            if (limit_access)
            {
                switch (lcd_status & 0x03)
                {
                case GPU_MODE_OAM:
                case GPU_MODE_VRAM:
                    logger->info("CPU cannot access OAM when GPU is using OAM, blocking CPU write");
                    return 0xFF;
                }
            }

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
			case 0xFF68:	return cgb_background_palette_index;
			case 0xFF69:
                // Block reading to VRAM when VRAM is being used
                if (gpu_mode == GPU_MODE_OAM || gpu_mode == GPU_MODE_VRAM)
                {
                    logger->info("Blocking read to 0xFF69 Background Palette Data, gpu_mode: {}", gpu_mode);
                    return 0xFF;
                }
                else
                {
                    return cgb_background_palette_data[cgb_background_palette_index];
                }
			case 0xFF6A:	return cgb_sprite_palette_index;
            case 0xFF6B:
                // Block reading to VRAM when VRAM is being used
                if (limit_access)
                {
                    switch (lcd_status & 0x03)
                    {
                    case GPU_MODE_OAM:
                    case GPU_MODE_VRAM:
                        logger->info("Blocking read to 0xFF6B Sprite Palette Data, gpu_mode: {}", gpu_mode);
                        return 0xFF;
                    }
                }

                return cgb_sprite_palette_data[cgb_sprite_palette_index];

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


void GPU::setByte(const uint16_t& pos, const uint8_t& val, const bool limit_access)
{
	uint8_t tile_block_num = 3;
    uint16_t source_address;
    uint16_t dest_address;

	switch (pos & 0xF000)
	{
	case 0x8000:
	case 0x9000:

        // Block writing to VRAM during Modes VRAM
        if (limit_access &&
           (lcd_status & 0x03) == GPU_MODE_VRAM)
        {
            logger->info("CPU cannot access OAM when GPU is using OAM, blocking CPU write");
            return;
        }

		vram_banks[curr_vram_bank % num_vram_banks][pos - 0x8000] = val;

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
		{   // Update Tiles[]
			Tile * tile = updateTile(pos, val, curr_vram_bank % num_vram_banks, tile_block_num);

            if (tile && is_color_gb && (curr_vram_bank % num_vram_banks) == 1)
            {   // Update CGB Tile attribute byte
                tile->setCGBAttribute(val);
            }
		}

		// Background Maps: 0x9800 - 0x9FFF (0x9800 - 0x9BFF, 0x9C00 - 0x9FFF)
		break;

	case 0xF000:
		if (pos < 0xFF00)
		{   // 0xFE00 - 0xFE9F : Sprite RAM (OAM)
            // Block writing to VRAM during Modes VRAM and OAM
            if (limit_access)
            {
                switch (lcd_status & 0x03)
                {
                case GPU_MODE_OAM:
                case GPU_MODE_VRAM:
                    logger->info("CPU cannot access OAM when GPU is using OAM, blocking CPU write");
                    return;
                }
            }

			object_attribute_memory[pos - 0xFE00] = val;
			break;
		}
		else if (pos < 0xFF77)
		{
			// LCD Stuff, VRAM bank selector

            // CGB only
            if (is_color_gb)
            {
                switch (pos)
                {
                case 0xFF4F:	curr_vram_bank = val & 0x01; return;    // Only bit 0 is writable
                case 0xFF51:	hdma1 = val; return;
                case 0xFF52:	hdma2 = val; return;
                case 0xFF53:	hdma3 = val; return;
                case 0xFF54:	hdma4 = val; return;
                case 0xFF55:
                    if ((val & BIT7) &&
                        gpu_mode == GPU_MODE_HBLANK)
                    {
                        logger->warn("Cannot start H-Blank DMA during H-Blank...");
                        hdma5 = 0xFF;
                        return;
                    }

                    if (cgb_dma_in_progress &&
                        cgb_dma_hblank_in_progress &&
                        (val & BIT7) == 0)
                    {   // Terminate current active H-Blank transfer
                        logger->warn("Terminating current H-Blank DMA, num bytes left to transfer: 0x{0:x}",
                            ((hdma5 & 0x7F) + 1) * 0x10);
                        hdma5 |= 0x80;
                        cgb_dma_in_progress = false;
                        cgb_dma_hblank_in_progress = false;
                        return;
                    }

                    hdma5 = val;
                    cgb_dma_in_progress = true;
                    
                    source_address = hdma1;
                    source_address = (source_address << 8) | hdma2;

                    dest_address = hdma3;
                    dest_address = (dest_address << 8) | hdma4;

                    memory->do_cgb_oam_dma_transfer(hdma1, hdma2, hdma3, hdma4, hdma5);
                    return;

                    // Background Palette Index
                case 0xFF68:
                    cgb_background_palette_index = val & 0x3F;
                    cgb_auto_increment_background_palette_index = val & 0x80;
                    return;

                    // Background Palette Data
                case 0xFF69:
                    cgb_background_palette_data[cgb_background_palette_index] = val;
                    updateBackgroundPalette(val);

                    if (cgb_auto_increment_background_palette_index)
                    {
                        cgb_background_palette_index++;
                    }
                    cgb_background_palette_index &= 0x3F;
                    return;

                    // Sprite Palette Index
                case 0xFF6A:
                    cgb_sprite_palette_index = val & 0x3F;
                    cgb_auto_increment_sprite_palette_index = val & 0x80;
                    return;

                    // Sprite Palette Data
                case 0xFF6B:
                    cgb_sprite_palette_data[cgb_sprite_palette_index] = val;
                    updateSpritePalette(val);
                    if (cgb_auto_increment_sprite_palette_index)
                    {
                        cgb_sprite_palette_index++;
                    }
                    cgb_sprite_palette_index &= 0x3F;
                    return;
                }
            }

			switch (pos)
			{
			case 0xFF40:	set_lcd_control(val); break;
            case 0xFF41:	set_lcd_status(val); break;
			case 0xFF42:	scroll_y = val; break;
			case 0xFF43:	scroll_x = val; break;
			case 0xFF44:
                if (lcd_display_enable == false)
                {   // Read only - Writing to this register resets the counter
                    logger->trace("Writing to 0xFF44, setting lcd_y to 0");
                    lcd_y = 0;
                }
                else
                {
                    logger->critical("yo");
                }
                break;
			case 0xFF45:
                lcd_y_compare = val;
                logger->trace("Setting lcd_y_compare: 0x{0:x} -> {0:d}", lcd_y_compare);
                break;
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


void GPU::set_color_palette(SDL_Color* palette, const uint8_t& val, bool zero_is_transparant)
{
    unsigned char color_val = 0;
    unsigned char alpha = 255;

    const Palette* tmp = nullptr;
    const CGBROMPalette cgb_rom_palette = get_cgb_rom_palette(
            get_cgb_rom_palette_ref(curr_opt_gb_palette));
    if (curr_opt_gb_palette != CGBPaletteCombo::NONE)
    {
        // Select correct palette to use
        if (palette == bg_palette_color)
        {
            tmp = &cgb_rom_palette.bg;
        }
        else if (palette == object_palette0_color)
        {
            tmp = &cgb_rom_palette.obj0;
        }
        else if (palette == object_palette1_color)
        {
            tmp = &cgb_rom_palette.obj1;
        }
    }

    is_tile_palette_updated = true;

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

        if (tmp == nullptr)
        {   // Update using default gray values
            switch ((val >> (i * 2)) & 3)
            {
            case 0: color_val = 255; break;
            case 1: color_val = 192; break;
            case 2: color_val = 96; break;
            case 3: color_val = 0; break;
            }
            palette[i] = { color_val, color_val, color_val, alpha };
        }
        else
        {   // Update using CGB ROM palette
            const uint32_t& use_color = tmp->colors[(val >> (i * 2)) & 3];
            // Get SDL_Color, update alpha
            SDL_Color sdl_color = get_sdl_color(use_color);
            sdl_color.a = alpha;
            palette[i] = sdl_color;
        }
    }
}

void GPU::use_color_palette(const CGBROMPalette& wanted_palette)
{
    object_palette0_color[0] = get_sdl_color(wanted_palette.obj0.colors[0]);
    object_palette0_color[1] = get_sdl_color(wanted_palette.obj0.colors[1]);
    object_palette0_color[2] = get_sdl_color(wanted_palette.obj0.colors[2]);
    object_palette0_color[3] = get_sdl_color(wanted_palette.obj0.colors[3]);

    object_palette1_color[0] = get_sdl_color(wanted_palette.obj1.colors[0]);
    object_palette1_color[1] = get_sdl_color(wanted_palette.obj1.colors[1]);
    object_palette1_color[2] = get_sdl_color(wanted_palette.obj1.colors[2]);
    object_palette1_color[3] = get_sdl_color(wanted_palette.obj1.colors[3]);

    bg_palette_color[0] = get_sdl_color(wanted_palette.bg.colors[0]);
    bg_palette_color[1] = get_sdl_color(wanted_palette.bg.colors[1]);
    bg_palette_color[2] = get_sdl_color(wanted_palette.bg.colors[2]);
    bg_palette_color[3] = get_sdl_color(wanted_palette.bg.colors[3]);
}

SDL_Color GPU::get_sdl_color(const uint32_t& color) const
{
    return SDL_Color {
        static_cast<uint8_t>((color >> 24) & 0xFF),
        static_cast<uint8_t>((color >> 16) & 0xFF),
        static_cast<uint8_t>((color >> 8) & 0xFF),
        static_cast<uint8_t>(color & 0xFF)
    };
}

void GPU::set_lcd_control(const uint8_t& lcdControl)
{
	lcd_control = lcdControl;
	bool old_lcd_display_enable     = lcd_display_enable;
    bool old_window_display_enable  = window_display_enable;

	lcd_display_enable =					(lcd_control & BIT7) ? true : false;
	window_tile_map_display_select.start =	(lcd_control & BIT6) ? 0x9C00 : 0x9800;
	window_tile_map_display_select.end =	(lcd_control & BIT6) ? 0x9FFF : 0x9BFF;
	window_display_enable =					(lcd_control & BIT5) ? true : false;
	bg_tile_data_select.start =				(lcd_control & BIT4) ? 0x8000 : 0x8800;
	bg_tile_data_select.end =				(lcd_control & BIT4) ? 0x8FFF : 0x97FF;
	bg_tile_data_select_method =			(lcd_control & BIT4) ? true : false;

	bg_tile_map_select.start =				(lcd_control & BIT3) ? 0x9C00 : 0x9800;
	bg_tile_map_select.end =				(lcd_control & BIT3) ? 0x9FFF : 0x9BFF;
	bg_tile_map_select_method =				(lcd_control & BIT3) ? true : false;
	object_size =							(lcd_control & BIT2) ? 16 : 8;
	object_display_enable =					(lcd_control & BIT1) ? true : false;
	bg_display_enable =						(lcd_control & BIT0) ? true : false;

    if (old_lcd_display_enable == true &&
        lcd_display_enable == false)
    {
        logger->info("Turning off display, setting lcd_y to 0");
        lcd_y = 0;
        update_lcd_status_coincidence_flag();
        set_lcd_status_mode_flag(GPU_MODE_VBLANK);
        ticks_accumulated = 0;
	}
	else if (old_lcd_display_enable == false &&
        lcd_display_enable == true)
	{
        logger->info("Turning display on, setting lcd_y to 0, old_lcd_display_enable: {0:b}, lcd_display_enable: {1:b}",
            old_lcd_display_enable,
            lcd_display_enable);
		lcd_y = 0;
		update_lcd_status_coincidence_flag();
        set_lcd_status_mode_flag(GPU_MODE_HBLANK);
        ticks_accumulated = 0;
	}

    if (lcd_display_enable == false)
    {
        //ticks_accumulated = 0;
        //set_lcd_status_mode_flag(GPU_MODE_HBLANK);
    }

    if (old_window_display_enable &&
        !window_display_enable &&
        lcd_y >= 0 &&
        lcd_y <= 143)
    {   // Turned off Window drawing
        logger->info("Turned off Window rendering while drawing mid-frame, lcd_y: {0:d}", lcd_y);
    }
    else if (!old_window_display_enable &&
        window_display_enable)
    {
        wait_frame_to_render_window = true;
        logger->info("Waiting one frame before enabling window display");
    }
}

void GPU::set_lcd_status(const uint8_t& lcdStatus)
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


void GPU::set_lcd_status_mode_flag(const GPU_MODE& mode)
{
    int prev_gpu_mode = gpu_mode;
    gpu_mode = mode;

    logger->debug("Changing GPU mode to: %s, previous GPU mode: %s",
        getGPUModeStr((GPU_MODE)prev_gpu_mode).c_str(),
        getGPUModeStr((GPU_MODE)gpu_mode).c_str());

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

void GPU::update_lcd_status_coincidence_flag()
{
    bool flag = (lcd_y == lcd_y_compare);

    // Update lcd_status' lcd_y == lcd_y_compare bit
    if (flag)
    {   // lcd_y == lcd_y_compare
        logger->debug("lcd_y == lcd_y_compare, lcd_y: 0x{0:x} -> {0:d}", lcd_y);

        lcd_status |= BIT2;

        if (lcd_status & BIT6)
        {
            logger->debug("Requesting LCDY Compare interrupt");
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
}

// Draws the 256x256 pixel Background to bg_frame[]
void GPU::renderFullBackgroundMap()
{
    int tile_map_vram_offset = bg_tile_map_select.start - 0x8000;
    int tile_offset = 0;
    int tile_offset_save = 0;
    int map_tile_num_offset = 0;
    int use_tile_num = 0;

    Tile *tile;
    uint8_t pixel_color;
    std::uint8_t tile_block_num;
    uint16_t row_pixel_offset;
    uint16_t use_row;
    uint8_t use_col;
    uint8_t pixel_use_row;
    uint8_t pixel_use_col;

    // CGB variables
    uint8_t cgb_tile_attributes = 0;
    uint8_t cgb_bg_palette_num;
    bool cgb_tile_vram_bank_num = 0;
    bool cgb_horizontal_flip;
    bool cgb_vertical_flip;
    bool cgb_bg_to_OAM_priority;

    for (uint16_t tile_map_pos = 0; tile_map_pos < bg_tile_map_select.end - bg_tile_map_select.start + 1; tile_map_pos++)
    {
        map_tile_num_offset = tile_map_vram_offset + tile_map_pos;

        // Get tile number from tile map
        use_tile_num = vram_banks[0][map_tile_num_offset];

        if (is_color_gb)
        {   // Get Tile's attributes
            cgb_tile_attributes = vram_banks[1][map_tile_num_offset];

            // Parse Tile's attributes
            cgb_bg_palette_num      = cgb_tile_attributes & 0x07;
            cgb_tile_vram_bank_num  = cgb_tile_attributes & BIT3;
            cgb_horizontal_flip     = cgb_tile_attributes & BIT5;
            cgb_vertical_flip       = cgb_tile_attributes & BIT6;
            cgb_bg_to_OAM_priority  = cgb_tile_attributes & BIT7;
        }

        // Find which tile block should be used
        tile_block_num = getTileBlockNum(use_tile_num);

        // Get the tile
        if (is_color_gb)
        {
            tile = getTileFromBGTiles(cgb_tile_vram_bank_num, tile_block_num, use_tile_num);
        }
        else
        {
            tile = getTileFromBGTiles(0, tile_block_num, use_tile_num);
        }

        const std::vector<uint8_t> & tile_data = tile->getRawPixelData();

        row_pixel_offset = ((tile_map_pos / 32) * 8 * 256);

        for (uint8_t row = 0; row < 8; row++)
        {
            use_row = (row * 256) + row_pixel_offset; // (0..65535)

            for (uint8_t col = 0; col < 8; col++)
            {
                use_col = col + ((tile_map_pos * 8) & 255); // (0..255)

                pixel_use_col = col;
                pixel_use_row = row;

                // Check if tile needs to be drawn flipped
                if (is_color_gb)
                {
                    if (cgb_vertical_flip)
                    {   // Vertically mirrored
                        pixel_use_row = 7 - row;
                    }

                    if (cgb_horizontal_flip)
                    {   // Horizontally mirrored
                        pixel_use_col = 7 - col;
                    }
                }

                // Get individual pixel color
                pixel_color = tile_data[pixel_use_col + (pixel_use_row * 8)];

                // Set pixel in frame
                if (is_color_gb)
                {
                    bg_frame[use_col + use_row] = cgb_background_palettes[cgb_bg_palette_num].getColor(pixel_color);
                }
                else
                {
                    bg_frame[use_col + use_row] = bg_palette_color[pixel_color];
                }
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
    uint16_t tile_map_offset;
    uint16_t use_tile_num;
    uint8_t tile_block_num;
    uint8_t curr_tile_col;
    uint8_t pixel_use_row;
    uint8_t pixel_use_col;

    // CGB variables
    uint8_t cgb_tile_attributes = 0;
    uint8_t cgb_bg_palette_num;
    bool cgb_tile_vram_bank_num = 0;
    bool cgb_horizontal_flip;
    bool cgb_vertical_flip;
    bool cgb_bg_to_OAM_priority;

    // Calculate which row of the Tile we're in (0..7)
    const uint8_t curr_tile_row = (lcd_y + scroll_y) & 0x07;

    // Get VRAM offset for which set of tiles to use
    const uint16_t tile_map_vram_offset = bg_tile_map_select.start - 0x8000;

    uint8_t use_pixel_x = scroll_x;
    const uint8_t use_pixel_y = scroll_y + lcd_y;     // Will rollover naturally due to uint8 (0..255)

    const uint16_t frame_y_offset = lcd_y * SCREEN_PIXEL_W;

    // Draw scanline
    std::lock_guard<std::mutex> lg(frame_mutex);
    for (uint8_t frame_x = 0; frame_x < SCREEN_PIXEL_W; frame_x++)
    {
        // Get tile offset in tile_map
        tile_map_offset = getTileMapNumber(use_pixel_x, use_pixel_y);

        // Get tile number from tile map
        use_tile_num = vram_banks[0][tile_map_vram_offset + tile_map_offset];

        if (is_color_gb)
        {   // Get Tile's attributes
            cgb_tile_attributes = vram_banks[1][tile_map_vram_offset + tile_map_offset];

            // Parse Tile's attributes
            cgb_bg_palette_num      = cgb_tile_attributes & 0x07;
            cgb_tile_vram_bank_num  = cgb_tile_attributes & BIT3;
            cgb_horizontal_flip     = cgb_tile_attributes & BIT5;
            cgb_vertical_flip       = cgb_tile_attributes & BIT6;
            cgb_bg_to_OAM_priority  = cgb_tile_attributes & BIT7;

            // Update BG to OAM array
            cgb_bg_to_oam_priority_array[frame_x] |= cgb_bg_to_OAM_priority;
        }

        // Find which tile memory block should be used
        tile_block_num = getTileBlockNum(use_tile_num);

        // Get the tile
        if (is_color_gb)
        {
            tile = getTileFromBGTiles(cgb_tile_vram_bank_num, tile_block_num, use_tile_num);

            // Save tile's most recent used ColorPalette
            tile->setCGBColorPalette(&cgb_background_palettes[cgb_bg_palette_num]);

            // Keep track of ColorPalette used at this pixel in the scanline
            cgb_bg_scanline_color_palettes[frame_x] = &cgb_background_palettes[cgb_bg_palette_num];
        }
        else
        {
            tile = getTileFromBGTiles(0, tile_block_num, use_tile_num);
        }

        // Calculate which col of the Tile we're in (0..7)
        curr_tile_col = (scroll_x + frame_x) & 0x07;

        pixel_use_row = curr_tile_row;
        pixel_use_col = curr_tile_col;

        // Check if tile needs to be drawn flipped
        if (is_color_gb)
        {
            if (cgb_vertical_flip)
            {   // Vertically mirrored
                pixel_use_row = 7 - curr_tile_row;
            }

            if (cgb_horizontal_flip)
            {   // Horizontally mirrored
                pixel_use_col = 7 - curr_tile_col;
            }
        }

        // Get pixel
        const uint8_t & pixel = tile->getPixel(pixel_use_row, pixel_use_col);

        if (frame_x + frame_y_offset > SCREEN_PIXEL_TOTAL)
        {
            use_pixel_x++;
            logger->error("frame_x + frame_y_offset > SCREEN_PIXEL_TOTAL, frame_x: {}, frame_y_offset: {}",
                frame_x,
                frame_y_offset);
            continue;
        }

        // Draw pixel
        if (is_color_gb)
        {
            frame[frame_x + frame_y_offset] = cgb_background_palettes[cgb_bg_palette_num].getColor(pixel);
        }
        else
        {
            frame[frame_x + frame_y_offset] = bg_palette_color[pixel];
        }

        // Will rollover naturally due to uint8 (0..255)
        use_pixel_x++;
    }
}

void GPU::drawWindowLine()
{
    Tile * tile;
    uint16_t tile_map_offset;
    uint16_t use_tile_num;
    uint8_t tile_block_num;
    uint8_t curr_tile_col;
    uint8_t pixel_use_row;
    uint8_t pixel_use_col;
    uint8_t use_pixel_x;;

    // CGB variables
    uint8_t cgb_tile_attributes = 0;
    uint8_t cgb_bg_palette_num;
    bool cgb_tile_vram_bank_num = 0;
    bool cgb_horizontal_flip;
    bool cgb_vertical_flip;
    bool cgb_bg_to_OAM_priority;

    // Calculate which row of the Tile we're in (0..7)
    uint8_t curr_tile_row = (lcd_y - window_y_pos) & 0x07;

    // Get VRAM offset for which set of tiles to use
    uint16_t tile_map_vram_offset = window_tile_map_display_select.start - 0x8000;

    uint8_t pixel_x_start = window_x_pos - 7;
    uint8_t use_pixel_y = lcd_y - window_y_pos;     // Will rollover naturally due to uint8 (0..255)

    uint16_t frame_y_offset = lcd_y * SCREEN_PIXEL_W;

    //if (use_pixel_y > SCREEN_PIXEL_H)
    //{
    //    use_pixel_y -= SCREEN_PIXEL_H;
    //}

    if (window_x_pos < 7)
    {
        pixel_x_start = 0;
    }

    //if (scroll_y - window_y_pos >= 144)
    //{
    //    return;
    //}

    if (window_x_pos >= 167)
    {
        return;
    }

    if (window_y_pos >= SCREEN_PIXEL_H)
    {
        return;
    }

    if (window_y_pos > lcd_y)
    {
        return;
    }

    //logger->info("Window frame_y_offset: {0:d}, use_pixel_y: {1:d}",
    //    frame_y_offset,
    //    use_pixel_y);

    if (use_pixel_y == 0 || use_pixel_y >= SCREEN_PIXEL_H)
    {

    }

    // Draw scanline
    std::lock_guard<std::mutex> lg(frame_mutex);
    for (uint8_t frame_x = pixel_x_start; frame_x < SCREEN_PIXEL_W; frame_x++)
    {
        use_pixel_x = frame_x - pixel_x_start;

        // Get tile in tile_map
        tile_map_offset = getTileMapNumber(use_pixel_x, use_pixel_y);

        // Get tile number from tile map
        use_tile_num = vram_banks[0][tile_map_vram_offset + tile_map_offset];

        if (is_color_gb)
        {   // Get Tile's attributes
            cgb_tile_attributes = vram_banks[1][tile_map_vram_offset + tile_map_offset];

            // Parse Tile's attributes
            cgb_bg_palette_num      = cgb_tile_attributes & 0x07;
            cgb_tile_vram_bank_num  = cgb_tile_attributes & BIT3;
            cgb_horizontal_flip     = cgb_tile_attributes & BIT5;
            cgb_vertical_flip       = cgb_tile_attributes & BIT6;
            cgb_bg_to_OAM_priority  = cgb_tile_attributes & BIT7;

            // Update BG to OAM array
            cgb_bg_to_oam_priority_array[frame_x] |= cgb_bg_to_OAM_priority;
        }

        // Find which tile memory block should be used
        tile_block_num = getTileBlockNum(use_tile_num);

        // Get the tile
        if (is_color_gb)
        {
            tile = getTileFromBGTiles(cgb_tile_vram_bank_num, tile_block_num, use_tile_num);

            // Save tile's most recent used ColorPalette
            tile->setCGBColorPalette(&cgb_background_palettes[cgb_bg_palette_num]);

            // Keep track of ColorPalette used at this pixel in the scanline
            cgb_bg_scanline_color_palettes[frame_x] = &cgb_background_palettes[cgb_bg_palette_num];
        }
        else
        {
            tile = getTileFromBGTiles(0, tile_block_num, use_tile_num);
        }

        // Calculate which col of the Tile we're in (0..7)
        curr_tile_col = use_pixel_x & 0x07;

        pixel_use_col = curr_tile_col;
        pixel_use_row = curr_tile_row;

        // Check if tile needs to be drawn flipped
        if (is_color_gb)
        {
            if (cgb_vertical_flip)
            {   // Vertically mirrored
                pixel_use_row = 7 - curr_tile_row;
            }

            if (cgb_horizontal_flip)
            {   // Horizontally mirrored
                pixel_use_col = 7 - curr_tile_col;
            }
        }

        // Get pixel
        const uint8_t & pixel = tile->getPixel(pixel_use_row, pixel_use_col);

        // Draw pixel
        if (is_color_gb)
        {
            frame[frame_x + frame_y_offset] = cgb_background_palettes[cgb_bg_palette_num].getColor(pixel);
        }
        else
        {
            frame[frame_x + frame_y_offset] = bg_palette_color[pixel];
        }
    }
}

void GPU::drawOAMLine()
{
    Tile * tile;
    std::uint8_t curr_sprite;
    std::uint8_t sprite_y, sprite_x, sprite_tile_num, byte3;
    uint8_t pixel_x, pixel_y;
    uint8_t tile_block_num;
    uint8_t use_x;
    bool cgb_tile_vram_bank_num = 0;
    uint8_t cgb_sprite_palette_num = 0;
    bool object_behind_bg, sprite_y_flip, sprite_x_flip, sprite_palette_num;
    uint8_t sprite_y_start, sprite_y_end;
    uint8_t num_x_pixels_to_draw = 8;

    pixel_x = pixel_y = 0;

    uint16_t frame_y_offset = lcd_y * SCREEN_PIXEL_W;

    if (!is_color_gb)
    {   // Sort OAM objects by X position (largest = lower priority = draw first)
        sortNonCGBOAMSpriteOrder();
    }

    std::lock_guard<std::mutex> lg(frame_mutex);

    //for (int i = 0; i < object_attribute_memory.size(); i += 4)
    for (const int & i : objects_pos_to_use)
    {
        //if (i % 2 == 1)
        //{
        //    continue;
        //}

        // Parse current sprite's 4 bytes of data
        curr_sprite     = i / 4;
        sprite_y        = object_attribute_memory[i] - 16;
        sprite_x        = object_attribute_memory[i + 1] - 8;
        sprite_tile_num = object_attribute_memory[i + 2];
        byte3           = object_attribute_memory[i + 3];

        object_behind_bg    = byte3 & BIT7;
        sprite_y_flip       = byte3 & BIT6;
        sprite_x_flip       = byte3 & BIT5;
        sprite_palette_num  = byte3 & BIT4; // Non CGB Mode only

        if (is_color_gb)
        {
            cgb_tile_vram_bank_num = byte3 & BIT3;
            cgb_sprite_palette_num = byte3 & 0x07;
        }

        // Calculate sprite_y_start and sprite_y_end
        sprite_y_end = sprite_y + object_size;  // Will naturally roll over
        
        if (object_attribute_memory[i] < 16)
        {
            sprite_y_start = 0;
        }
        else
        {
            sprite_y_start = sprite_y;
        }


        // Check if sprite is being drawn anywhere on the visible screen
        if (sprite_x >= 168 && sprite_x <= 248)
        {   // This sprite doesn't draw at all in 0..159, skip it
            continue;
        }

        // Check to see if sprite is rendered on current line (Y position)
        if (sprite_y_start <= lcd_y && sprite_y_end > lcd_y)
        {

            //logger->info("Drawing OAM sprite: %{0:d},\tlcd_y: %{1:d}",
            //    curr_sprite,
            //    lcd_y);

            // Check for case of object_size = 16, ie sprite size is 8x16
            // Then check if we should be using the next sprite 8x8 sprite to draw
            if (object_size == 16)
            {
                uint8_t curr_sprite_y = lcd_y - sprite_y;

                // the sprite is not flipped and
                // curr_sprite_y > 7
                if (!sprite_y_flip && curr_sprite_y > 7)
                {   // Select bottom 8x8 sprite
                    sprite_tile_num |= 0x01;
                }   // the sprite is flipped and curr_sprite_y <= 7
                else if (sprite_y_flip && curr_sprite_y < 8)
                {   // Select bottom 8x8 sprite
                    sprite_tile_num |= 0x01;
                }
                else
                {   // Select upper 8x8 sprite
                    sprite_tile_num &= 0xFE;    // Last bit is ignored for upper 8x8 tile
                }
            }

            // Find which tile block should be used
            tile_block_num = getSpriteTileBlockNum(sprite_tile_num);

            // Get the tile
            if (is_color_gb)
            {
                tile = getTileFromBGTiles(cgb_tile_vram_bank_num, tile_block_num, sprite_tile_num);

                // Save tile's most recent used ColorPalette
                tile->setCGBColorPalette(&cgb_sprite_palettes[cgb_sprite_palette_num]);
            }
            else
            {
                tile = getTileFromBGTiles(0, tile_block_num, sprite_tile_num);
            }

            // Draw row of pixels
            for (uint8_t x = 0; x < 8; x++)
            {
                use_x = x + sprite_x;

                // Check to make sure sprite X position isn't out of bounds
                // i.e. only draw from 0..159
                if (use_x >= SCREEN_PIXEL_W)
                {
                    continue;
                }

                // Get current pixel in frame
                auto curr_frame_pixel = frame[use_x + frame_y_offset];
                bool bg_is_color_0 = false;

                // Check if pixel should be drawn due to object_behind_bg flag
                // or CGB's object_behind_bg flag
                if (is_color_gb)
                {   // Ensure that we have a BG palette present
                    if (cgb_bg_scanline_color_palettes[use_x])
                    {
                        bg_is_color_0 = SDLColorsAreEqual(curr_frame_pixel, cgb_bg_scanline_color_palettes[use_x]->getColor(0));
                    }

                    if (cgb_bg_to_oam_priority_array[use_x] == 0)
                    {   // Use OAM byte 3 flag
                        if (object_behind_bg)
                        {   // BG has priority over sprite
                            if (bg_is_color_0 == false)
                            {   // Current BG pixel == color 1, 2, or 3 - don't draw sprite here
                                continue;
                            }
                        }
                    }
                    else
                    {   // BG has priority over sprite
                        continue;
                    }
                }
                else if (object_behind_bg)
                {   // Non-CGB handling
                    bg_is_color_0 = SDLColorsAreEqual(curr_frame_pixel, bg_palette_color[0]);
                    if (bg_is_color_0 == false)
                    {   // Current BG pixel == color 1, 2, or 3 - don't draw sprite here
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
                    if (pixel_y > 7)
                    {
                        pixel_y -= 8;
                    }
                }

                // Get color for current pixel in object
                const uint8_t & pixel_color = tile->getPixel(pixel_y, pixel_x);

                if (pixel_color == 0)
                {
                    continue;   // Color 0 == transparent == don't display
                }

                // Draw sprite pixel to frame
                if (is_color_gb)
                {   // Draw sprite in color
                    frame[use_x + frame_y_offset] = cgb_sprite_palettes[cgb_sprite_palette_num].getColor(pixel_color);
                }
                else
                {   // Draw sprite in gray scale
                    if (sprite_palette_num == 0)
                    {
                        frame[use_x + frame_y_offset] = object_palette0_color[pixel_color];
                    }
                    else
                    {
                        frame[use_x + frame_y_offset] = object_palette1_color[pixel_color];
                    }
                }

            } // end for(x)
        } // end if(y)
    } //end for(sprite)
}

void GPU::renderLine()
{
    if (lcd_y > 144)
    {
        return;
    }

    logger->debug("lcd_y: {}", lcd_y);

    if (bg_display_enable)
    {
        drawBackgroundLine();
    }

    if (window_display_enable && !wait_frame_to_render_window)
    {
        drawWindowLine();
    }

    if (object_display_enable)
    {
        drawOAMLine();
    }

    if (is_color_gb)
    {   // Clear current scanline's background to OAM priority array
        cgb_bg_to_oam_priority_array.fill(0);
        cgb_bg_scanline_color_palettes.fill(NULL);
    }
}

uint16_t GPU::getTileMapNumber(const uint8_t& pixel_x, const uint8_t& pixel_y) const
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

uint8_t GPU::getTileBlockNum(const int& use_tile_num) const
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

uint8_t GPU::getSpriteTileBlockNum(const int& use_tile_num) const
{
    return use_tile_num / 128;
}

Tile * GPU::getTileFromBGTiles(const uint8_t& use_vram_bank, const uint8_t& tile_block_num, const int& use_tile_num)
{
    if (use_tile_num < 128)
    {
        return &bg_tiles[use_vram_bank][tile_block_num][use_tile_num];
    }
    else
    {
        return &bg_tiles[use_vram_bank][tile_block_num][use_tile_num - 128];
    }
}

void GPU::run(const uint8_t & cpuTickDiff)
{
    if (lcd_display_enable == false)
    {
        if ((lcd_status & 0x03) != GPU_MODE_HBLANK)
        {
            set_lcd_status_mode_flag(GPU_MODE_HBLANK);
        }
        return;
    }

	ticks_accumulated += cpuTickDiff;

    switch (lcd_status & 0x03)
	{
	case GPU_MODE_HBLANK:

		if (ticks_accumulated >= 204)
		{
            if (lcd_y == 0)
            {
                logger->trace("Start Frame");
            }

			lcd_y++;
            logger->trace("Incrementing lcd_y: 0x{0:x} -> {0:d}", lcd_y);
            update_lcd_status_coincidence_flag();

            if (cgb_dma_in_progress &&
                cgb_dma_hblank_in_progress &&
                hdma5 != 0xFF)
            {
                memory->do_cgb_h_blank_dma(hdma1, hdma2, hdma3, hdma4, hdma5);
            }

			// Check if frame rendering has completed, start VBLANK interrupt
            if (lcd_y == 144)
            {
                if (render_full_frame)
                {
                    if (bg_frame.empty())
                    {
                        bg_frame.resize(TOTAL_SCREEN_PIXEL_W * TOTAL_SCREEN_PIXEL_H);
                    }

                    renderFullBackgroundMap();
                }
                memory->interrupt_flag |= INTERRUPT_VBLANK;
                set_lcd_status_mode_flag(GPU_MODE_VBLANK);
            }
            else
            {
                set_lcd_status_mode_flag(GPU_MODE_OAM);
            }

			ticks_accumulated = 0;
		}
		break;

	case GPU_MODE_VBLANK:

		if (ticks_accumulated >= 456)
		{
			lcd_y++;
            logger->trace("Incrementing lcd_y: 0x{0:x} -> {0:d}", lcd_y);
            update_lcd_status_coincidence_flag();

			if (lcd_y > 153)
			{   // Copy frame into curr_frame for use by external programs
                std::lock_guard<std::mutex> lg(frame_mutex);
                std::memcpy(curr_frame, frame, sizeof(SDL_Color) * SCREEN_PIXEL_W * SCREEN_PIXEL_H);
                frame_is_ready = true;
				lcd_y = 0;
                update_lcd_status_coincidence_flag();
                set_lcd_status_mode_flag(GPU_MODE_OAM);
				logger->trace("End Frame");

                if (wait_frame_to_render_window)
                {   // Done waiting for frame to finish, can now enable window display
                    wait_frame_to_render_window = false;
                }
			}

            ticks_accumulated = 0;
		}
		break;


	case GPU_MODE_OAM:

		if (ticks_accumulated >= 80)
		{
            set_lcd_status_mode_flag(GPU_MODE_VRAM);
            ticks_accumulated = 0;
		}
		break;


	case GPU_MODE_VRAM:

		if (ticks_accumulated >= 172)
		{
            logger->trace("Rendering line lcd_y: 0x{0:x} -> {0:d}", lcd_y);
            renderLine();
            set_lcd_status_mode_flag(GPU_MODE_HBLANK);
			ticks_accumulated = 0;
		}
		break;
	}
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


Tile * GPU::updateTile(const uint16_t& pos, const uint8_t& val,
  const bool& use_vram_bank, const uint8_t& tile_block_num)
{
	Tile *tile = NULL;
	uint16_t tile_num;
	uint16_t byte_pos;
	uint16_t offset;

	offset = 0x8000 + (0x800 * tile_block_num);

	if (pos - offset >= 0)
	{
		byte_pos = pos - offset;
		tile_num = std::floor(byte_pos / NUM_BYTES_PER_TILE);
		tile = &bg_tiles[use_vram_bank][tile_block_num][tile_num];
        /*if (tile_num == 2)
        {
            // Update byte_pos with correct offset
            offset = 0x9000;
            byte_pos = pos - offset;
            tile->updateRawData(byte_pos % 16, val);
        }
        else*/
        {
            tile->updateRawData(byte_pos % 16, val);
        }
        bg_tiles_updated = true;
	}
    return tile;
}

std::vector<std::vector<std::vector<Tile>>>& GPU::getBGTiles()
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

void GPU::updateBackgroundPalette(const uint8_t& val)
{
    // Get Color Palette object
    ColorPalette & colorPalette = cgb_background_palettes[cgb_background_palette_index / 8];

    // Update color palette
    colorPalette.updateRawByte(cgb_background_palette_index % 8, val);

    is_cgb_tile_palette_updated = true;

    logger->info("Updating Background color palette {0:x} with val {1:x}",
        cgb_background_palette_index,
        val);
}

void GPU::updateSpritePalette(const uint8_t& val)
{
    // Get Color Palette object
    ColorPalette & colorPalette = cgb_sprite_palettes[cgb_sprite_palette_index / 8];

    // Update color palette
    colorPalette.updateRawByte(cgb_sprite_palette_index % 8, val);

    is_cgb_tile_palette_updated = true;

    logger->info("Updating Sprite color palette {0:x} with val {1:x}",
        cgb_sprite_palette_index,
        val);
}

// Sorts OAM objects by X position (largest = lower priority = draw first)
void GPU::sortNonCGBOAMSpriteOrder()
{
    std::vector<uint8_t> indices(OAM_NUM_SPRITES);
    std::vector<uint8_t> xPositions(OAM_NUM_SPRITES);

    // Get list of X positions
    for (int i = 0; i < OAM_NUM_SPRITES; i++)
    {
        xPositions[i] = object_attribute_memory[(i * 4) + 1];
        indices[i] = i;
    }

    // Get indices of sorted vector
    std::sort(indices.begin(), indices.end(), [&](const uint8_t & i, const uint8_t & j)
    {
        return xPositions[i] > xPositions[j];
    });

    // Update object_pos_to_use by indices
    for (int i = 0; i < xPositions.size(); i++)
    {
        objects_pos_to_use[i] = indices[i] * 4;
    }
}

std::array<SDL_Color, SCREEN_PIXEL_TOTAL> GPU::getFrame() const
{
    std::array<SDL_Color, SCREEN_PIXEL_TOTAL> array;
    std::copy(curr_frame, curr_frame + (SCREEN_PIXEL_TOTAL),
        array.data());
    return array;
}

std::string GPU::getGPUModeStr(const GPU_MODE& mode) const
{
    switch (mode)
    {
    case GPU_MODE::GPU_MODE_HBLANK: return "HBLANK";
    case GPU_MODE::GPU_MODE_VBLANK: return "VBLANK";
    case GPU_MODE::GPU_MODE_OAM:    return "OAM";
    case GPU_MODE::GPU_MODE_VRAM:   return "VRAM";
    case GPU_MODE::GPU_MODE_NONE:   return "NONE";
    default: return "Unknown";
    }
}

CGBROMPalette GPU::get_cgb_rom_palette(const uint8_t& ref) const
{
    Palette bg, obj0, obj1;
    switch (ref)
    {   // UP
        case 0x12:
            obj0 =  { WHTCLR_U32, 0xFFAD63FF, 0x843100FF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0xFFAD63FF, 0x843100FF, BLACK_U32 };
            bg =    { WHITE_U32,  0xFFAD63FF, 0x843100FF, BLACK_U32 };
            break;

        // UP + A
        case 0xB0:
            obj0 =  { WHTCLR_U32, 0x7BFF31FF, 0x008400FF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0x63A5FFFF, 0x0000FFFF, BLACK_U32 };
            bg =    { WHITE_U32,  0xFF8484FF, 0x943A3AFF, BLACK_U32 };
            break;

        // UP + B
        case 0x79:
            obj0 =  { WHTCLR_U32, 0xFFAD63FF, 0x843100FF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0xFFAD63FF, 0x843100FF, BLACK_U32 };
            bg =    { 0xFFE6C5FF, 0xCE9C84FF, 0x846B29FF, 0x5A3108FF };
            break;

        // LEFT
        case 0xB8:
            obj0 =  { WHTCLR_U32, 0xFF8484FF, 0x943A3AFF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0x7BFF31FF, 0x008400FF, BLACK_U32 };
            bg =    { WHITE_U32,  0x63A5FFFF, 0x0000FFFF, BLACK_U32 };
            break;

        // LEFT + A
        case 0xAD:
            obj0 =  { WHTCLR_U32, 0xFF8484FF, 0x943A3AFF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0xFFAD63FF, 0x843100FF, BLACK_U32 };
            bg =    { WHITE_U32,  0x8C8CDEFF, 0x52528CFF, BLACK_U32 };
            break;

        // LEFT + B
        case 0x16:  // Gray
            obj0 =  { WHTCLR_U32, 0xA5A5A5FF, 0x525252FF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0xA5A5A5FF, 0x525252FF, BLACK_U32 };
            bg =    { WHITE_U32,  0xA5A5A5FF, 0x525252FF, BLACK_U32 };
            break;

        // DOWN
        case 0x17:
            obj0 =  { 0xFFFFA500, 0xFF9494FF, 0x9494FFFF, BLACK_U32 };
            obj1 =  { 0xFFFFA500, 0xFF9494FF, 0x9494FFFF, BLACK_U32 };
            bg =    { 0xFFFFA5FF, 0xFF9494FF, 0x9494FFFF, BLACK_U32 };
            break;

        // DOWN + A
        case 0x07:
            obj0 =  { WHTCLR_U32, YELLOW_U32, RED_U32, BLACK_U32 };
            obj1 =  { WHTCLR_U32, YELLOW_U32, RED_U32, BLACK_U32 };
            bg =    { WHITE_U32,  YELLOW_U32, RED_U32, BLACK_U32 };
            break;

        // DOWN + B
        case 0xBA:
            obj0 =  { WHTCLR_U32, 0x63A5FFFF, 0x0000FFFF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0x7BFF31FF, 0x008400FF, BLACK_U32 };
            bg =    { WHITE_U32,  YELLOW_U32, 0x7B4A00FF, BLACK_U32 };
            break;

        // RIGHT
        case 0x05:
            obj0 =  { WHTCLR_U32, 0x52FF00FF, 0xFF4200FF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0x52FF00FF, 0xFF4200FF, BLACK_U32 };
            bg =    { WHITE_U32,  0x52FF00FF, 0xFF4200FF, BLACK_U32 };
            break;

        // RIGHT + A
        case 0x7C:
            obj0 =  { WHTCLR_U32, 0xFF8484FF, 0x943A3AFF, BLACK_U32 };
            obj1 =  { WHTCLR_U32, 0xFF8484FF, 0x943A3AFF, BLACK_U32 };
            bg =    { WHITE_U32,  0x7BFF31FF, 0x0063C5FF, BLACK_U32 };
            break;

        // RIGHT + B
        case 0x13:
            obj0 =  { BLKCLR_U32, 0x008484FF, 0xFFDE00FF, WHITE_U32 };
            obj1 =  { BLKCLR_U32, 0x008484FF, 0xFFDE00FF, WHITE_U32 };
            bg =    { BLKCLR_U32, 0x008484FF, 0xFFDE00FF, WHITE_U32 };
            break;

        // DMG Green (LAST)
        case 0xFE:
            obj0 =  { 0x9BBC0FFF, 0x8BAC0FFF, 0x306230FF, 0x0F380FFF };
            obj1 =  { 0x9BBC0FFF, 0x8BAC0FFF, 0x306230FF, 0x0F380FFF };
            bg =    { 0x9BBC0FFF, 0x8BAC0FFF, 0x306230FF, 0x0F380FFF };
            break;

        default:
            return GetUniqueGBPalette(ref);
    }

    return CGBROMPalette { obj0, obj1, bg };
}

void GPU::changeCGBPalette()
{
    // Loop color palettes when changed
    if (curr_opt_gb_palette == CGBPaletteCombo::LAST)
    {
        curr_opt_gb_palette = CGBPaletteCombo::NONE;
    }
    else
    {   // Increment CGB ROM palette
        curr_opt_gb_palette = (CGBPaletteCombo)((int)curr_opt_gb_palette + 1);
    }

    // Update BG, OBJ0, and OBJ1 palettes with new color scheme
    set_color_palette(bg_palette_color, bg_palette);
    set_color_palette(object_palette0_color, object_pallete0, true);
    set_color_palette(object_palette1_color, object_pallete1, true);
}

uint8_t GPU::get_cgb_rom_palette_ref(const CGBPaletteCombo& ref) const
{
    switch (curr_opt_gb_palette)
    {
        case CGBPaletteCombo::NONE:     return 0;
        case CGBPaletteCombo::UP:       return 0x12;
        case CGBPaletteCombo::UP_A:     return 0xB0;
        case CGBPaletteCombo::UP_B:     return 0x79;
        case CGBPaletteCombo::LEFT:     return 0xB8;
        case CGBPaletteCombo::LEFT_A:   return 0xAD;
        case CGBPaletteCombo::LEFT_B:   return 0x16;
        case CGBPaletteCombo::DOWN:     return 0x17;
        case CGBPaletteCombo::DOWN_A:   return 0x07;
        case CGBPaletteCombo::DOWN_B:   return 0xBA;
        case CGBPaletteCombo::RIGHT:    return 0x05;
        case CGBPaletteCombo::RIGHT_A:  return 0x7C;
        case CGBPaletteCombo::RIGHT_B:  return 0x13;
        case CGBPaletteCombo::CUSTOM:   return memory->cartridgeReader->game_title_hash;
        case CGBPaletteCombo::LAST:     return 0xFE;
        default:
            return 0xFF;
    }
}