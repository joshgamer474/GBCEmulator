#include "stdafx.h"

#include "GPU.h"
#include "Memory.h"
#include "CPU.h"

GPU::GPU()
{
	is_color_gb = false;
	num_vram_banks = 1;
	curr_vram_bank = 0;
	ticks = 0;

	lcd_control = NULL;

	vram_banks.resize(num_vram_banks, std::vector<unsigned char>(VRAM_SIZE, 0));
	object_attribute_memory.resize(OAM_SIZE);

	background_palette_data.resize(PALETTE_DATA_SIZE);
	sprite_palette_data.resize(PALETTE_DATA_SIZE);

	gpu_mode = GPU_MODE_VRAM;
}


GPU::~GPU()
{

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
				printf("WARNING - GPU::readByte() doesn't handle address: %#06x\n", pos);
				return -1;
			}
		}

	default:
		printf("WARNING - GPU::readByte() doesn't handle address: %#06x\n", pos);
		return -1;
	}
}


void GPU::setByte(std::uint16_t pos, std::uint8_t val)
{
	switch (pos & 0xF000)
	{
	case 0x8000:
	case 0x9000:

		vram_banks[curr_vram_bank][pos - 0x8000] = val;
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
			switch (pos)
			{
			case 0xFF40:	set_lcd_control(val); break;
			case 0xFF41:	lcd_status = val; break;
			case 0xFF42:	scroll_y = val; break;
			case 0xFF43:	scroll_x = val; break;
			case 0xFF44:	lcd_y = 0; break;				// Read only - Writing to this register resets the counter
			case 0xFF45:	lcd_y_compare = val; break;
			case 0xFF46:	memory->do_oam_dma_transfer(val); break;
			case 0xFF47:	bg_palette = val; break;
			case 0xFF48:	object_pallete0 = val; break;
			case 0xFF49:	object_pallete1 = val; break;
			case 0xFF4A:	window_y_pos = val; break;
			case 0xFF4B:	window_x_pos = val; break;
			case 0xFF4F:	curr_vram_bank = val; break;
			case 0xFF51:	hdma1 = val; break;
			case 0xFF52:	hdma2 = val; break;
			case 0xFF53:	hdma3 = val; break;
			case 0xFF54:	hdma4 = val; break;
			case 0xFF55:	hdma5 = val; break;
			case 0xFF68:	background_palette_index = val; break;
			case 0xFF69:	background_palette_data[background_palette_index] = val; break;
			case 0xFF6A:	sprite_palette_index = val; break;
			case 0xFF6B:	sprite_palette_data[sprite_palette_index] = val; break;

			default:
				printf("WARNING - GPU::setByte() doesn't handle address: %#06x\n", pos);
			}
		}
		break;
	default:
		printf("WARNING - GPU::setByte() doesn't handle address: %#06x\n", pos);
	}
}



void GPU::set_lcd_control(unsigned char lcdControl)
{
	lcd_control = lcdControl;

	lcd_display_enable =					(lcd_control & 0x80) ? true : false;
	window_tile_map_display_select.start =	(lcd_control & 0x40) ? 0x9C00 : 0x9800;
	window_tile_map_display_select.end =	(lcd_control & 0x40) ? 0x9FFF : 0x9BFF;
	window_display_enable =					(lcd_control & 0x20) ? true : false;
	bg_tile_data_select.start =				(lcd_control & 0x10) ? 0x8000 : 0x8800;
	bg_tile_data_select.end =				(lcd_control & 0x10) ? 0x8FFF : 0x97FF;

	bg_tile_map_select.start =				(lcd_control & 0x08) ? 0x9C00 : 0x9800;
	bg_tile_map_select.end =				(lcd_control & 0x08) ? 0x9FFF : 0x9BFF;
	object_size =							(lcd_control & 0x04) ? 16 : 8;
	object_display_enable =					(lcd_control & 0x02) ? true : false;
	bg_display_enable =						(lcd_control & 0x01) ? true : false;
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
	lcd_status |= mode;
}

void GPU::set_lcd_status_coincidence_flag(bool flag)
{
	if (flag)
		lcd_status |= 0x04;
	else
		lcd_status &= 0xFB;
}

void GPU::getTile(int tile_num, int line_num, TILE *tile)
{
	int pos = (tile_num * 16) + (line_num * 2);
	tile->b0 = vram_banks[curr_vram_bank][pos];
	tile->b1 = vram_banks[curr_vram_bank][pos + 1];
}

void GPU::renderLine()
{
	int pixelOffset = lcd_y * SCREEN_PIXEL_W;

	// VRAM offset for which set of tiles to use
	//int tile_map_offset = bg_tile_data_select.start - 0x8000;
	int tile_map_offset = bg_tile_map_select.start - 0x8000;


	// Which tile to start with in the map line
	std::uint16_t tile_offset = (scroll_x >> 3);
	std::uint16_t old_tile_offset = tile_offset;

	// Which line of pixels to use in the tiles
	int y = (lcd_y + scroll_y) & 0x07;

	// Where in the tile line to start
	int x = scroll_x & 0x07;

	int tile_num;
	std::uint8_t color;
	TILE tile;

	if (lcd_y == 0)
		y_roll_over = 0;
	else if (y == 0)
		y_roll_over++;

	// Which line of tiles to use
	//tile_map_offset += (((lcd_y + scroll_y) & 0xFF) >> 3) (y_roll_over * (SCREEN_PIXEL_W / 8));
	tile_map_offset += (y_roll_over * (SCREEN_PIXEL_W / 8));

	tile_num = vram_banks[curr_vram_bank][tile_map_offset + tile_offset];
	getTile(tile_num, y, &tile);

	if (bg_display_enable)
	{
		for (int i = 0; i < SCREEN_PIXEL_W; i++)
		{
			color = ((tile.b1 >> (6 - x)) & 0x02) | ((tile.b0 >> (7 - x)) & 0x01);
			frame[i + (lcd_y * SCREEN_PIXEL_W)] = palette_color[color];

			x++;
			if (x == 8)
			{
				x = 0;
				tile_offset += 1;
				tile_offset &= 31;	// Max 32 tiles
				tile_num = vram_banks[curr_vram_bank][tile_map_offset + tile_offset];
				getTile(tile_num, y, &tile);
			}
		}
	}

	if (object_display_enable)
	{

	}

}


void GPU::display()
{
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glRasterPos2f(-1, 1);
	glPixelZoom(1, -1);
	glDrawPixels(SCREEN_PIXEL_W, SCREEN_PIXEL_H, GL_RGB, GL_UNSIGNED_BYTE, frame);
	//printFrame();
	glFlush();
	glutSwapBuffers();
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
			lcd_y++;

			// Check if frame rendering has completed, start VBLANK interrupt
			if (lcd_y == 143)
			{
				if (memory->interrupt_enable & INTERRUPT_VBLANK)
					memory->interrupt_flag |= INTERRUPT_VBLANK;
				gpu_mode = GPU_MODE_VBLANK;
			}
			else
			{
				gpu_mode = GPU_MODE_OAM;
			}

			ticks -= 204;
		}
		break;


	case GPU_MODE_VBLANK:

		if (ticks >= 456)
		{
			lcd_y++;
			//glutPostRedisplay();

			if (lcd_y > 153)
			{
				display();
				lcd_y = 0;
				gpu_mode = GPU_MODE_OAM;
			}

			ticks -= 456;
		}
		break;


	case GPU_MODE_OAM:

		if (ticks >= 80)
		{
			gpu_mode = GPU_MODE_VRAM;
			ticks -= 80;
		}
		break;


	case GPU_MODE_VRAM:

		if (ticks >= 172)
		{
			renderLine();
			gpu_mode = GPU_MODE_HBLANK;
			ticks -= 172;
		}
		break;
	}

	if (lcd_y == lcd_y_compare)
		set_lcd_status_coincidence_flag(true);
	else
		set_lcd_status_coincidence_flag(false);
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
		printf(s.c_str());
		printf("\n");
	}
}