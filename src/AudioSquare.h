#ifndef AUDIO_SQUARE_H
#define AUDIO_SQUARE_H

#include <array>
#include <memory>
#include <string>
#include <vector>

class AudioSquare
{
public:
    AudioSquare(const uint16_t & register_offset);
    virtual ~AudioSquare();

    void setByte(const uint16_t & addr, const uint8_t & val);
    uint8_t readByte(const uint16_t & addr);
    void tick();
    void tickLengthCounter();

    uint8_t curr_sample;
    uint8_t duty_pos;
    bool is_enabled;

private:
    void parseRegister(const uint8_t & reg, const uint8_t & val);
    void reset();
    uint8_t getWaveDuty();
    void generateOutputClock();
 
    std::array<uint8_t, 5> registers;
    uint8_t sweep_shift_num;
    uint8_t wave_pattern_duty;
    uint8_t sound_length_data;
    uint8_t initial_volume_of_envelope;
    uint8_t envelope_sweep_num;
    uint16_t reg_offset;
    uint16_t frequency_16;
    uint32_t frequency;
    uint64_t timer;
    uint64_t period;
    float sweep_time;
    float sound_length;
    bool sweep_decrease;
    bool envelope_increase;
    bool stop_output_when_sound_length_ends;
    bool restart_sound;
};

#endif