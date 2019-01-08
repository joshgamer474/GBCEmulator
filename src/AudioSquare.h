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
    void tickVolumeEnvelope();

    uint8_t duty_pos;
    uint8_t output_volume;
    bool is_enabled;

private:
    void initWaveDutyTable();
    void parseRegister(const uint8_t & reg, const uint8_t & val);
    void reset();
 
    std::array<uint8_t, 5> registers;
    std::array<std::array<bool, 8>, 4> wave_duty_table;
    uint8_t volume;
    uint8_t curr_sample;
    uint8_t sweep_shift_num;
    uint8_t wave_pattern_duty;
    uint8_t sound_length_data;
    uint8_t initial_volume_of_envelope;
    uint8_t envelope_period_load;
    uint8_t envelope_period;
    uint16_t reg_offset;
    uint16_t frequency_16;
    uint32_t frequency;
    uint64_t timer;
    uint64_t period;
    float sweep_time;
    float sound_length;
    bool sweep_decrease;
    bool envelope_increase;
    bool envelope_running;
    bool stop_output_when_sound_length_ends;
    bool restart_sound;
};

#endif