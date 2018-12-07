#ifndef APU_H
#define APU_H

#include <array>
#include <memory>
#include <AudioSquare.h>
#include <AudioWave.h>
#include <AudioNoise.h>

class APU
{
public:
    APU();
    virtual ~APU();

    void setByte(const uint16_t & addr, const uint8_t & val);
    uint8_t readByte(const uint16_t & addr);
    void run();

private:
    std::unique_ptr<AudioSquare> sound_channel_1;
    std::unique_ptr<AudioSquare> sound_channel_2;
    std::unique_ptr<AudioWave>   sound_channel_3;
    std::unique_ptr<AudioNoise>  sound_channel_4;
    uint8_t channel_control;
    uint8_t selection_of_sound_output;
    uint8_t sound_on_off;
    std::array<uint8_t, 16> wave_pattern_RAM;
};

#endif