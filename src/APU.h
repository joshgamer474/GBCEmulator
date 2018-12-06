#ifndef APU_H
#define APU_H

#include <array>

struct SoundChannel1 {
    std::array<uint8_t, 5> registers;
};

struct SoundChannel2 {
    std::array<uint8_t, 4> registers;
};

struct SoundChannel3 {
    std::array<uint8_t, 5> registers;
    std::array<uint8_t, 16> wave_pattern_RAM;
};

struct SoundChannel4 {
    std::array<uint8_t, 4> registers;
};

class APU
{
public:
    APU();
    virtual ~APU();

    void setByte(uint16_t addr, uint8_t val);
    uint8_t readByte(uint16_t addr);

private:
    SoundChannel1 sound_channel_1;
    SoundChannel2 sound_channel_2;
    SoundChannel3 sound_channel_3;
    SoundChannel4 sound_channel_4;
    uint8_t channel_control;
    uint8_t selection_of_sound_output;
    uint8_t sound_on_off;
};

#endif