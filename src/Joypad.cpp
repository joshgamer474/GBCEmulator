#include "stdafx.h"
#include "Joypad.h"
#include "Debug.h"

Joypad::Joypad()
{
	joypad_byte = 0;
}

Joypad::~Joypad()
{
    logger.reset();
}


void Joypad::set_joypad_button(BUTTONS button)
{
	std::uint8_t val = 0;

	if (button <= RIGHT)
	{
		val |= BIT5;
	}
	else
	{
		val |= BIT5;
	}

	switch (button)
	{
	case DOWN:
	case START:
		val |= BIT3;
		break;
		
	case UP:
	case SELECT:
		val |= BIT2;
		break;

	case LEFT:
	case B:
		val |= BIT1;
		break;

	case RIGHT:
	case A:
		val |= BIT0;
		break;
	}

	set_joypad_byte(val);
}

void Joypad::set_joypad_byte(std::uint8_t val)
{
	joypad_byte = val;
    hasInterrupt = true;
}

std::uint8_t Joypad::get_joypad_byte()
{
	return joypad_byte;
}

void Joypad::check_keyboard_input(SDL_Event *e)
{
	BUTTONS b = NONE;
	if (e->key.keysym.sym == SDLK_RIGHT)
		b = RIGHT;
	else if (e->key.keysym.sym == SDLK_LEFT)
		b = LEFT;
	else if (e->key.keysym.sym == SDLK_UP)
		b = UP;
	else if (e->key.keysym.sym == SDLK_DOWN)
		b = DOWN;
	else if (e->key.keysym.sym == SDLK_RETURN)
		b = START;
	else if (e->key.keysym.sym == SDLK_z)
		b = A;
	else if (e->key.keysym.sym == SDLK_x)
		b = B;

	if (b != NONE)
		set_joypad_button(b);
}