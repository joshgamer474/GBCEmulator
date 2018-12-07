#include <AudioWave.h>

AudioWave::AudioWave(const uint16_t & register_offset)
    : reg_offset(register_offset)
{
    output_level    = 0;
    frequency_16    = 0;
    frequency       = 0;
    sound_length    = 0.0f;
    sound_enabled   = false;
    restart_sound   = false;
    stop_output_when_sound_length_ends = false;
}

AudioWave::~AudioWave()
{

}

void AudioWave::setByte(const uint16_t & addr, const uint8_t & val)
{
    uint16_t useAddr = addr - reg_offset;

    if (useAddr < registers.size())
    {
        registers[useAddr] = val;
        parseRegister(useAddr, val);
    }
    else
    {

    }
}

uint8_t AudioWave::readByte(const uint16_t & addr)
{
    uint16_t useAddr = addr - reg_offset;

    if (useAddr < registers.size())
    {
        return registers[useAddr];
    }
    else
    {
        return 0xFF;
    }
}


void AudioWave::parseRegister(const uint8_t & reg, const uint8_t & val)
{
    switch (reg)
    {
    case 0: sound_enabled = val & 0x80; break;
    case 1: sound_length = (256.0f - val) * (1.0f / 256.0f); break;
    case 2: output_level = (val & 0x60) >> 5; break;
    case 3:
        frequency_16 &= 0xF0;
        frequency_16 |= val;
        break;
    case 4:
        frequency_16 &= 0x0F;
        frequency_16 |= (static_cast<uint16_t>(val) & 0x03) << 8;
        stop_output_when_sound_length_ends = val & 0x40;
        restart_sound = val & 0x80;

        // Calculate frequency
        frequency = 131072 / (2048 - frequency_16);
        break;
    }
}