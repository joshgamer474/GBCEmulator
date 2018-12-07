#include "APU.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>

APU::APU()
{
    channel_control = 0;
    selection_of_sound_output = 0;
    sound_on_off = 0;

    sound_channel_1 = std::make_unique<AudioSquare>(0xFF10, "audio_out_square_1.pcm");
    sound_channel_2 = std::make_unique<AudioSquare>(0xFF15, "audio_out_square_2.pcm");
    sound_channel_3 = std::make_unique<AudioWave>(0xFF1A);
    sound_channel_4 = std::make_unique<AudioNoise>(0xFF20);
}

APU::~APU()
{

}

void APU::setByte(const uint16_t & addr, const uint8_t & val)
{
    if (addr >= 0xFF10 && addr <= 0xFF14)
    {   // Channel 1 - Square 1
        sound_channel_1->setByte(addr, val);
    }
    else if (addr >= 0xFF16 && addr <= 0xFF19)
    {   // Channel 2 - Square 2
        sound_channel_2->setByte(addr, val);
    }
    else if (addr >= 0xFF1A && addr <= 0xFF1E)
    {   // Channel 3 - Wave
        sound_channel_3->setByte(addr, val);
    }
    else if (addr >= 0xFF20 && addr <= 0xFF23)
    {   // Channel 4 - Noise
        sound_channel_4->setByte(addr, val);
    }
    else if (addr >= 0xFF30 && addr <= 0xFF3F)
    {   // Wave Pattern RAM
        wave_pattern_RAM[addr - 0xFF30] = val;
    }

    switch (addr)
    {
    case 0xFF24: channel_control            = val; break;
    case 0xFF25: selection_of_sound_output  = val; break;
    case 0xFF26: sound_on_off               = val; break;
    }
}

uint8_t APU::readByte(const uint16_t & addr)
{
    if (addr >= 0xFF10 && addr <= 0xFF14)
    {
        return sound_channel_1->readByte(addr);
    }
    else if (addr >= 0xFF16 && addr <= 0xFF19)
    {
        return sound_channel_2->readByte(addr);
    }
    else if (addr >= 0xFF1A && addr <= 0xFF1E)
    {
        return sound_channel_3->readByte(addr);
    }
    else if (addr >= 0xFF20 && addr <= 0xFF23)
    {
        return sound_channel_4->readByte(addr);
    }
    else if (addr >= 0xFF30 && addr <= 0xFF3F)
    {
        return wave_pattern_RAM[addr - 0xFF30];
    }

    switch (addr)
    {
    case 0xFF24: return channel_control;
    case 0xFF25: return selection_of_sound_output;
    case 0xFF26: return sound_on_off;
    }

    return 0xFF;
}

void APU::run()
{
    sound_channel_1->run();
    sound_channel_2->run();
}