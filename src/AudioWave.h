#ifndef AUDIO_WAVE_H
#define AUDIO_WAVE_H

#include <array>

class AudioWave
{
public:
    AudioWave(const uint16_t & register_offset);
    virtual ~AudioWave();


    void setByte(const uint16_t & addr, const uint8_t & val);
    uint8_t readByte(const uint16_t & addr);

private:
    void parseRegister(const uint8_t & reg, const uint8_t & val);
    
    std::array<uint8_t, 5> registers;
    uint16_t reg_offset;

    uint8_t output_level = 0;
    uint16_t frequency_16 = 0;
    uint32_t frequency = 0;
    float sound_length = 0.0f;
    bool sound_enabled = false;
    bool restart_sound = false;
    bool stop_output_when_sound_length_ends = false;
};

#endif