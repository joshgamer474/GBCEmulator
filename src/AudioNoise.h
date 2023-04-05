#ifndef AUDIO_NOISE_H
#define AUDIO_NOISE_H

#include <cstdint>
#include <array>
#include <memory>
#include <spdlog/spdlog.h>

class AudioNoise
{
public:
    AudioNoise(const uint16_t & register_offset, std::shared_ptr<spdlog::logger> logger);
    virtual ~AudioNoise();
    AudioNoise& operator=(const AudioNoise& rhs);

    void setByte(const uint16_t & addr, const uint8_t & val);
    uint8_t readByte(const uint16_t & addr) const;
    void tick();
    void tickLengthCounter();
    void tickVolumeEnvelope();
    void reset();
    bool isRunning();

    std::shared_ptr<spdlog::logger> logger;
    uint8_t output_volume;
    uint8_t sound_length_data;
    bool is_enabled;
    bool restart_sound;

private:
    void parseRegister(const uint8_t & reg, const uint8_t & val);
    void reloadPeriod(uint8_t & period, const uint8_t & periodLoad);

    uint16_t reg_offset;
    std::array<uint8_t, 8> divisors;
    uint16_t timer;
    uint8_t initial_volume_of_envelope;
    uint8_t envelope_period;
    uint8_t envelope_period_load;
    uint8_t shift_clock_frequency;
    uint8_t dividing_ratio_of_frequencies;
    uint8_t volume;
    uint16_t lfsr;
    bool half_counter_step;
    bool dac_enabled;
    bool envelope_increase;
    bool envelope_running;
    bool stop_output_when_sound_length_ends;
};

#endif