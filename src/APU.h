#ifndef APU_H
#define APU_H

#include <array>
#include <memory>
#include <fstream>
#include <AudioSquare.h>
#include <AudioWave.h>
#include <AudioNoise.h>
#include <spdlog/spdlog.h>

#define SAMPLE_RATE 44100
//#define SAMPLE_BUFFER_SIZE 1024
#define SAMPLE_OUTPUT_CHANNEL_SIZE 2
#define SAMPLE_BUFFER_SIZE 770
#define SAMPLE_BUFFER_MEM_SIZE SAMPLE_BUFFER_SIZE * SAMPLE_OUTPUT_CHANNEL_SIZE
#define SAMPLE_BUFFER_MEM_SIZE_FLOAT SAMPLE_BUFFER_MEM_SIZE * sizeof(float)

#define USE_FLOAT

struct Sample {
#ifndef USE_FLOAT
    uint8_t left   = 0;
    uint8_t right  = 0;
#else
    float left = 0;
    float right = 0;
#endif

    std::string getSampleStr()
    {
        return std::string("{" + std::to_string(left) + ", " + std::to_string(right) + "}, ");
    }
};

class APU
{
public:
    APU();
    virtual ~APU();

    void setByte(const uint16_t & addr, const uint8_t & val);
    uint8_t readByte(const uint16_t & addr);
    void run(const uint64_t & cpuTicks);

    std::shared_ptr<spdlog::logger> logger;
    uint64_t samplesPerFrame;
    uint8_t sdl_silence_val;
    uint32_t audio_device_id;

private:
    void initSDLAudio();
    bool isSoundOutLeft(uint8_t sound_number);
    bool isSoundOutRight(uint8_t sound_number);
    void logSamples();

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
    uint16_t sample_buffer_counter;
    uint16_t frame_sequence_timer_val;
    uint16_t frame_sequence_timer;
    uint64_t curr_apu_ticks;
    uint64_t prev_cpu_ticks;
    uint64_t sample_timer;
    uint64_t sample_timer_val;
    bool left_out_enabled;
    bool right_out_enabled;
};

#endif