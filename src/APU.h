#ifndef APU_H
#define APU_H

#include <array>
#include <memory>
#include <fstream>
#include <AudioSquare.h>
#include <AudioWave.h>
#include <AudioNoise.h>

#define SAMPLE_RATE 44100
#define SAMPLE_BUFFER_SIZE 1024
#define SAMPLE_BUFFER_UINT8_SIZE SAMPLE_BUFFER_SIZE * 4

struct Sample {
    uint16_t left   = 0;
    uint16_t right  = 0;
};

class APU
{
public:
    APU();
    virtual ~APU();

    void setByte(const uint16_t & addr, const uint8_t & val);
    uint8_t readByte(const uint16_t & addr);
    void run(const uint64_t & cpuTicks);

private:
    bool isSoundOutLeft(uint8_t sound_number);
    bool isSoundOutRight(uint8_t sound_number);

    std::unique_ptr<std::ofstream> outRightChannel;
    std::unique_ptr<std::ofstream> outLeftChannel;
    std::unique_ptr<AudioSquare> sound_channel_1;
    std::unique_ptr<AudioSquare> sound_channel_2;
    std::unique_ptr<AudioWave>   sound_channel_3;
    std::unique_ptr<AudioNoise>  sound_channel_4;
    std::vector<Sample> sample_buffer;
    uint8_t frame_sequence_step;
    uint8_t left_volume;
    uint8_t right_volume;
    uint8_t channel_control;
    uint8_t selection_of_sound_output;
    uint8_t sound_on;
    uint16_t frame_sequence_timer_val;
    uint16_t frame_sequence_timer;
    uint32_t audio_device_id;
    uint64_t curr_apu_ticks;
    uint64_t prev_cpu_ticks;
    uint64_t sample_timer;
    uint64_t sample_timer_val;
    bool left_out_enabled;
    bool right_out_enabled;
};

#endif