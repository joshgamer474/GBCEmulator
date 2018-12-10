#ifndef AUDIO_WAVE_H
#define AUDIO_WAVE_H

#include <array>
#include <vector>

class AudioWave
{
public:
    AudioWave(const uint16_t & register_offset);
    virtual ~AudioWave();


    void setByte(const uint16_t & addr, const uint8_t & val);
    uint8_t readByte(const uint16_t & addr);
    std::vector<uint8_t> run();

    bool is_enabled;

private:
    void parseRegister(const uint8_t & reg, const uint8_t & val);
    void generateOutputClock();
    uint32_t getSoundLength();
    
    std::vector<uint8_t> curr_sample;
    std::array<uint8_t, 5> registers;
    uint16_t reg_offset;

    uint8_t sound_length;
    uint8_t output_level;
    uint16_t frequency_16;
    uint32_t frequency;
    uint64_t timer;
    uint64_t period;
    float sound_length_seconds;
    bool restart_sound;
    bool stop_output_when_sound_length_ends;
    std::array<uint8_t, 16> wave_pattern_RAM;
};

#endif