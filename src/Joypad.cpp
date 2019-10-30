#ifdef _WIN32
#include "stdafx.h"
#endif // _WIN32

#include "Joypad.h"
#include "Debug.h"

Joypad::Joypad(std::shared_ptr<spdlog::logger> _logger)
    : logger(_logger)
{
	joypad_byte     = 0xFF;
    joypad_state    = 0xFF;
    hasInterrupt    = false;
}

Joypad::~Joypad()
{
    logger.reset();
}

Joypad& Joypad::operator=(const Joypad& rhs)
{   // Copy from rhs
    joypad_byte     = rhs.joypad_byte;
    joypad_state    = rhs.joypad_state;
    hasInterrupt    = rhs.joypad_state;

    return *this;
}

void Joypad::set_joypad_button(BUTTON button)
{
    uint8_t bitToUnset = 0;
    bool previouslyNotSet = false;
    bool requestInterrupt = false;

    switch (button)
    {
    case RIGHT:     bitToUnset = BIT0; break;
    case LEFT:      bitToUnset = BIT1; break;
    case UP:        bitToUnset = BIT2; break;
    case DOWN:      bitToUnset = BIT3; break;
    case A:         bitToUnset = BIT4; break;
    case B:         bitToUnset = BIT5; break;
    case SELECT:    bitToUnset = BIT6; break;
    case START:     bitToUnset = BIT7; break;
    default:
        return;
    }

    // Get previous state of button
    if (bitIsUnset(joypad_state, bitToUnset) == false)
    {
        previouslyNotSet = true;
    }

    joypad_state = unsetBit(joypad_state, bitToUnset);

    logger->trace("Button {} pressed", button);

    if (bitIsUnset(joypad_byte, BIT4) &&
        buttonIsDirectionKey(button))
    {
        requestInterrupt = true;
    }
    else if (bitIsUnset(joypad_byte, BIT5) &&
        buttonIsButtonKey(button))
    {
        requestInterrupt = true;
    }

    if (requestInterrupt && !previouslyNotSet)
    {
        hasInterrupt = true;
    }
}

void Joypad::release_joypad_button(BUTTON button)
{
    switch (button)
    {
    case RIGHT:     joypad_state |= BIT0; break;
    case LEFT:      joypad_state |= BIT1; break;
    case UP:        joypad_state |= BIT2; break;
    case DOWN:      joypad_state |= BIT3; break;
    case A:         joypad_state |= BIT4; break;
    case B:         joypad_state |= BIT5; break;
    case SELECT:    joypad_state |= BIT6; break;
    case START:     joypad_state |= BIT7; break;
    }

    logger->trace("Button {} released", button);
}

void Joypad::set_joypad_byte(std::uint8_t val)
{
	joypad_byte = val | 0xCF; // Bits 0-3 are read-only, bits 6-7 are 1
}

std::uint8_t Joypad::get_joypad_byte()
{
    uint8_t out = joypad_byte;
    uint8_t joypad_last_4_bits = 0x00;

    if (bitIsUnset(joypad_byte, BIT4))
    {
        joypad_last_4_bits = joypad_state & 0x0F;
    }
    else if (bitIsUnset(joypad_byte, BIT5))
    {
        joypad_last_4_bits = (joypad_state >> 4) & 0x0F;
    }

    out &= 0xF0;
    out |= joypad_last_4_bits;

    return out;
}

void Joypad::check_keyboard_input(SDL_Event *e)
{
	BUTTON b = NONE;
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

uint8_t Joypad::unsetBit(const uint8_t byte, const uint8_t bitToUnset) const
{
    uint8_t bit_flipped = bitToUnset ^ 0xFF;
    return byte & bit_flipped;
}

bool Joypad::bitIsUnset(const uint8_t byte, const uint8_t bitSet) const
{
    const uint8_t byte_flipped = byte ^ 0xFF;
    return byte_flipped & bitSet;
}

bool Joypad::buttonIsDirectionKey(const BUTTON b) const
{
    return b <= DOWN;
}

bool Joypad::buttonIsButtonKey(const BUTTON b) const
{
    return b >= A;
}

bool Joypad::buttonIsSet(const BUTTON& button) const
{
    switch (button)
    {
    case RIGHT:     return joypad_state & BIT0;
    case LEFT:      return joypad_state & BIT1;
    case UP:        return joypad_state & BIT2;
    case DOWN:      return joypad_state & BIT3;
    case A:         return joypad_state & BIT4;
    case B:         return joypad_state & BIT5;
    case SELECT:    return joypad_state & BIT6;
    case START:     return joypad_state & BIT7;
    default:        return false;
    }
}

uint8_t Joypad::getJoypadState() const
{
    return joypad_state;
}