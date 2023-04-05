#ifndef APU_H
#define APU_H

#include <atomic>
#include <array>
#include <fstream>
#include <functional>
#include <memory>

#include <AudioSquare.h>
#include <AudioWave.h>
#include <AudioNoise.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <SDL_audio.h>
#include <RollingAvg.h>

#define SAMPLE_RATE 44100
#define MICROSEC_PER_FRAME (1.0 / 60.0) * 1000.0 * 1000.0


#define USE_FLOAT
//#define WRITE_AUDIO_OUT

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

    uint8_t* data()
    {
        return reinterpret_cast<uint8_t *>(this);
    }
};

class APU
{
public:
    APU(std::shared_ptr<spdlog::sinks::rotating_file_sink_st> logger_sink, std::shared_ptr<spdlog::logger> logger);
    virtual ~APU();
    APU& operator=(const APU& rhs);

    void setByte(const uint16_t & addr, const uint8_t & val);
    uint8_t readByte(const uint16_t & addr) const;
    void run(const uint8_t & cpuTicks);
    void initCGB();
    void setChannelLogLevel(spdlog::level::level_enum level);
    void setSampleUpdateMethod(std::function<void(float, int)> function);
    void sendSamplesToDebugger(bool b);
    void writeSamplesOutAsync(const uint32_t& audio_device);
    void sleepUntilBufferIsEmpty(const std::chrono::duration<double>& frame_start_time);

    std::shared_ptr<spdlog::logger> logger;
    uint16_t samplesPerFrame;
    uint8_t sdl_silence_val;
    uint32_t audio_device_id;
    const int SAMPLE_BUFFER_SIZE;             // 1470
    const int SAMPLE_OUTPUT_CHANNEL_SIZE;     // 2
    const int SAMPLE_BUFFER_MEM_SIZE;         // SAMPLE_BUFFER_SIZE * SAMPLE_OUTPUT_CHANNEL_SIZE
    const int SAMPLE_BUFFER_MEM_SIZE_FLOAT;   // SAMPLE_BUFFER_MEM_SIZE * sizeof(float)

private:
    void initSDLAudio();
    void reset();
    bool isSoundOutLeft(uint8_t sound_number) const;
    bool isSoundOutRight(uint8_t sound_number) const;
    void sendChannelOutputToSample(Sample & sample, const uint8_t & audio, const uint8_t & channelNum) const;
    void sendChannelOutputToSampleFloat(Sample & sample, const float & audio, const uint8_t & channelNum) const;
    void writeSamplesOut(const uint32_t & audio_device, const std::vector<Sample>& samples, const uint16_t num_samples);
    uint8_t mixAudio(const uint8_t & audio1, const uint8_t & audio2);
    void logSamples();
    void clearCurrentAudioBuffer();

    std::function<void(float, int)> sendSampleUpdate;
    std::unique_ptr<std::ofstream> audioFileOut;
    std::unique_ptr<AudioSquare> sound_channel_1;
    std::unique_ptr<AudioSquare> sound_channel_2;
    std::unique_ptr<AudioWave>   sound_channel_3;
    std::unique_ptr<AudioNoise>  sound_channel_4;
    std::array<std::vector<Sample>, 2> double_sample_buffer;
    uint8_t frame_sequence_step;
    uint8_t left_volume;
    uint8_t right_volume;
    uint8_t left_volume_use;
    uint8_t right_volume_use;
    uint8_t channel_control;
    uint8_t selection_of_sound_output;
    uint16_t sample_buffer_counter;
    const uint16_t frame_sequence_timer_val;
    uint16_t frame_sequence_timer;
    uint32_t sample_timer;
    uint32_t prev_sample_size;
    RollingAvg rolling_avg_sample_size;
    const uint32_t sample_timer_val;
    SDL_AudioSpec desired_spec;
    SDL_AudioSpec obtained_spec;
    bool sound_on;
    bool left_out_enabled;
    bool right_out_enabled;
    bool double_speed_mode;
    bool send_samples_to_debugger;
    bool initialized;
    std::atomic_bool curr_sample_buffer;
};

#endif