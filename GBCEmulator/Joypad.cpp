#include "stdafx.h"
#include "Joypad.h"
#include "Memory.h"

Joypad::Joypad()
{
	memory = NULL;
	joypad_byte = 0;
}

Joypad::~Joypad()
{

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

	//if (memory->interrupt_enable & INTERRUPT_JOYPAD)
	//	memory->interrupt_flag |= INTERRUPT_JOYPAD;
	memory->interrupt_flag |= INTERRUPT_JOYPAD;
}

std::uint8_t Joypad::get_joypad_byte()
{
	return joypad_byte;
}

void Joypad::check_keyboard_input()
{
	BUTTONS b = NONE;
	if (GetKeyState(VK_RIGHT) & 0x8000)
		b = RIGHT;
	else if (GetKeyState(VK_LEFT) & 0x8000)
		b = LEFT;
	else if (GetKeyState(VK_UP) & 0x8000)
		b = UP;
	else if (GetKeyState(VK_DOWN) & 0x8000)
		b = DOWN;
	else if (GetKeyState(VK_RETURN) & 0x8000)
		b = START;
	else if (GetKeyState('A') & 0x8000)
		b = A;
	else if (GetKeyState('B') & 0x8000)
		b = B;

	if (b != NONE)
		set_joypad_button(b);
}