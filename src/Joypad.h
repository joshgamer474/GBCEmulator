#ifndef JOYPAD_H
#define JOYPAD_H

#ifdef _WIN32
#include <Windows.h>
#endif

#include <cstdint>
#include <SDL.h>
#include <spdlog/spdlog.h>

#define BIT0 0x01
#define BIT1 0x02
#define BIT2 0x04
#define BIT3 0x08
#define BIT4 0x10
#define BIT5 0x20
#define BIT6 0x40
#define BIT7 0x80

class Joypad
{
public:
	Joypad(std::shared_ptr<spdlog::logger> logger);
	~Joypad();
    Joypad& operator=(const Joypad& rhs);

    enum BUTTON : int
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

	void set_joypad_button(BUTTON button);
    void release_joypad_button(BUTTON button);
	void set_joypad_byte(uint8_t val);
	uint8_t get_joypad_byte();
	void check_keyboard_input(SDL_Event *e);
    uint8_t unsetBit(const uint8_t byte, const uint8_t bitToUnset) const;
    bool bitIsUnset(const uint8_t byte, const uint8_t bitSet) const;
    bool buttonIsDirectionKey(const BUTTON b) const;
    bool buttonIsButtonKey(const BUTTON b) const;
    bool buttonIsSet(const BUTTON& b) const;
    uint8_t getJoypadState() const;

	std::shared_ptr<spdlog::logger> logger;
    bool hasInterrupt;

private:
	uint8_t joypad_byte;
    uint8_t joypad_state;
};

#endif