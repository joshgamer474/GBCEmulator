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
    void tick();
    void tickLengthCounter();
    void reset();

    uint8_t output_volume;
    bool is_enabled;

private:
    void parseRegister(const uint8_t & reg, const uint8_t & val);
    void updateSample();

    uint16_t reg_offset;

    uint8_t curr_sample;
    uint8_t volume;
    uint8_t sound_length_data;
    uint8_t nibble_pos;
    uint16_t frequency_16;
    uint32_t frequency;
    uint64_t timer;
    uint64_t period;
    float sound_length_seconds;
    bool channel_is_enabled;
    bool restart_sound;
    bool stop_output_when_sound_length_ends;
    std::array<uint8_t, 16> wave_pattern_RAM;
};

#endif