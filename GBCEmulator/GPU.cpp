#include "stdafx.h"

#include "GPU.h"

GPU::GPU()
{
	is_color_gb = false;
	num_vram_banks = 1;
	curr_vram_bank = 0;

	lcd_control = NULL;

	vram_banks.resize(num_vram_banks, std::vector<unsigned char>(VRAM_SIZE, 0));
	object_attribute_memory.resize(OAM_SIZE);

	background_palette_data.resize(PALETTE_DATA_SIZE);
	sprite_palette_data.resize(PALETTE_DATA_SIZE);
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
			case 0xFF40:	lcd_control = val;
			case 0xFF41:	lcd_status = val;
			case 0xFF42:	scroll_y = val; break;
			case 0xFF43:	scroll_x = val; break;
			case 0xFF44:	lcd_y = val; break;
			case 0xFF45:	lcd_y_compare = val; break;
			case 0xFF46:	oam_dma = val; break;
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

	default:
		printf("WARNING - GPU::setByte() doesn't handle address: %#06x\n", pos);
	}
}




void GPU::display()
{
	glClear(GL_COLOR_BUFFER_BIT);
	glutSwapBuffers();
	glFlush();
}


