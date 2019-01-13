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
    void tickSweep();
    void reset();

    uint8_t duty_pos;
    uint8_t output_volume;
    uint8_t sound_length_data;
    bool is_enabled;

private:
    uint16_t calculateSweepFrequency();
    void initWaveDutyTable();
    void parseRegister(const uint8_t & reg, const uint8_t & val);
    void reloadPeriod(uint8_t & period, const uint8_t & periodLoad);
 
    std::array<std::array<bool, 8>, 4> wave_duty_table;
    uint8_t volume;
    uint8_t curr_sample;
    uint8_t sweep_period;
    uint8_t sweep_period_load;
    uint8_t sweep_shift;
    uint8_t wave_pattern_duty;
    uint8_t initial_volume_of_envelope;
    uint8_t envelope_period_load;
    uint8_t envelope_period;
    uint16_t reg_offset;
    uint16_t frequency_16;
    uint16_t sweep_frequency_16;
    uint64_t timer;
    uint64_t period;
    float sound_length;
    bool sweep_decrease;
    bool sweep_running;
    bool envelope_increase;
    bool envelope_running;
    bool envelope_enabled;
    bool stop_output_when_sound_length_ends;
    bool restart_sound;
};

#endif