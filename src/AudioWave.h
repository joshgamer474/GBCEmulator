#ifndef AUDIO_WAVE_H
#define AUDIO_WAVE_H

#include <array>
#include <vector>
#include <memory>
#include <spdlog/spdlog.h>

class AudioWave
{
public:
    AudioWave(const uint16_t & register_offset, std::shared_ptr<spdlog::logger> logger, const bool is_color_gb);
    virtual ~AudioWave();
    AudioWave& operator=(const AudioWave& rhs);

    void setByte(const uint16_t & addr, const uint8_t & val, const bool& extra_length_clock);
    uint8_t readByte(const uint16_t & addr) const;
    void powerOn();
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
    uint8_t byte_pos;
    uint8_t access_ticks;
    uint16_t frequency_16;
    uint32_t frequency;
    uint64_t timer;
    uint64_t period;
    float sound_length_seconds;
    bool channel_is_enabled;
    bool stop_output_when_sound_length_ends;
    bool is_color_gb;
    std::array<uint8_t, 16> wave_pattern_RAM;
};

#endif