#include "APU.h"

APU::APU()
{
    channel_control = 0;
    selection_of_sound_output = 0;
    sound_on_off = 0;
}

APU::~APU()
{

}

void APU::setByte(uint16_t addr, uint8_t val)
{
    if (addr >= 0xFF10 && addr <= 0xFF14)
    {
        sound_channel_1.registers[addr - 0xFF10] = val;
    }
    else if (addr >= 0xFF16 && addr <= 0xFF19)
    {
        sound_channel_2.registers[addr - 0xFF16] = val;
    }
    else if (addr >= 0xFF1A && addr <= 0xFF1E)
    {
        sound_channel_3.registers[addr - 0xFF1A] = val;
    }
    else if (addr >= 0xFF20 && addr <= 0xFF23)
    {
        sound_channel_4.registers[addr - 0xFF20] = val;
    }
    else if (addr >= 0xFF30 && addr <= 0xFF3F)
    {
        sound_channel_3.wave_pattern_RAM[addr - 0xFF30] = val;
    }

    switch (addr)
    {
    case 0xFF24: channel_control = val; break;
    case 0xFF25: selection_of_sound_output = val; break;
    case 0xFF26: sound_on_off = val; break;
    }
}

uint8_t APU::readByte(uint16_t addr)
{
    if (addr >= 0xFF10 && addr <= 0xFF14)
    {
        return sound_channel_1.registers[addr - 0xFF10];
    }
    else if (addr >= 0xFF16 && addr <= 0xFF19)
    {
        return sound_channel_2.registers[addr - 0xFF16];
    }
    else if (addr >= 0xFF1A && addr <= 0xFF1E)
    {
        return sound_channel_3.registers[addr - 0xFF1A];
    }
    else if (addr >= 0xFF20 && addr <= 0xFF23)
    {
        return sound_channel_4.registers[addr - 0xFF20];
    }
    else if (addr >= 0xFF30 && addr <= 0xFF3F)
    {
        return sound_channel_3.wave_pattern_RAM[addr - 0xFF30];
    }

    switch (addr)
    {
    case 0xFF24: return channel_control;
    case 0xFF25: return selection_of_sound_output;
    case 0xFF26: return sound_on_off;
    }

    return 0xFF;
}