#ifndef AUDIO_NOISE_H
#define AUDIO_NOISE_H

#include <array>

class AudioNoise
{
public:
    AudioNoise(const uint16_t & register_offset);
    virtual ~AudioNoise();

    void setByte(const uint16_t & addr, const uint8_t & val);
    uint8_t readByte(const uint16_t & addr);

    bool is_enabled;

private:
    void parseRegister(const uint8_t & reg, const uint8_t & val);
    
    std::array<uint8_t, 4> registers;
    uint16_t reg_offset;

    uint8_t initial_volume_of_envelope = 0;
    uint8_t envelope_sweep_num = 0;
    uint8_t shift_clock_frequency = 0;
    uint8_t counter_step = 0;
    float sound_length = 0.0f;
    float dividing_ratio_of_frequencies = 0.0f;
    float frequency = 0.0f;
    bool envelope_increase = false;
    bool stop_output_when_sound_length_ends = false;
    bool restart_sound = false;
};

#endif