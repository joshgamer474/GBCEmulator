#ifndef AUDIO_WAVE_H
#define AUDIO_WAVE_H

#include <array>
#include <vector>
#include <memory>
#include <spdlog/spdlog.h>

class AudioWave
{
public:
    AudioWave(const uint16_t & register_offset, std::shared_ptr<spdlog::logger> logger);
    virtual ~AudioWave();

    void setByte(const uint16_t & addr, const uint8_t & val);
    uint8_t readByte(const uint16_t & addr);
    void tick();
    void tickLengthCounter();
    void reset();
    bool isRunning();

    std::shared_ptr<spdlog::logger> logger;
    uint8_t output_volume;
    uint8_t sound_length_load;
    uint16_t sound_length_data;
    bool is_enabled;
    bool restart_sound;

private:
    void updateSample();

    uint16_t reg_offset;
    uint8_t curr_sample;
    uint8_t volume;
    uint8_t nibble_pos;
    uint16_t frequency_16;
    uint32_t frequency;
    uint64_t timer;
    uint64_t period;
    float sound_length_seconds;
    bool channel_is_enabled;
    bool stop_output_when_sound_length_ends;
    std::array<uint8_t, 16> wave_pattern_RAM;
};

#endif