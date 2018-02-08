#pragma once
#ifndef JOYPAD_H
#define JOYPAD_H

#include <cstdint>
#include <Windows.h>

#define BIT0 0x01
#define BIT1 0x02
#define BIT2 0x04
#define BIT3 0x08
#define BIT4 0x10
#define BIT5 0x20
#define BIT6 0x40
#define BIT7 0x80

class Memory;

class Joypad
{
public:
	Joypad();
	~Joypad();

	Memory *memory;

	enum BUTTONS
	{
		DOWN,
		UP,
		LEFT,
		RIGHT,
		START,
		SELECT,
		B,
		A,
		NONE
	};

	void set_joypad_button(BUTTONS button);
	void set_joypad_byte(std::uint8_t val);
	std::uint8_t get_joypad_byte();
	void check_keyboard_input();

private:

	unsigned char joypad_byte;
};
#endif