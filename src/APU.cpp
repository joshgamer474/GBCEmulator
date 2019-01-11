#include "APU.h"
#include <GBCEmulator.h>
#include <CPU.h>
#include <Joypad.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>

APU::APU()
{
    frame_sequence_step     = 0;
    channel_control         = 0;
    selection_of_sound_output = 0;
    sound_on                = 0;
    curr_apu_ticks          = 0;
    prev_cpu_ticks          = 0;
    left_volume             = 0;
    right_volume            = 0;
    left_volume_use         = 0;
    right_volume_use        = 0;
    sample_buffer_counter   = 0;
    samplesPerFrame         = 0;
    left_out_enabled        = false;
    right_out_enabled       = false;

    sound_channel_1 = std::make_unique<AudioSquare>(0xFF10);
    sound_channel_2 = std::make_unique<AudioSquare>(0xFF15);
    sound_channel_3 = std::make_unique<AudioWave>(0xFF1A);
    sound_channel_4 = std::make_unique<AudioNoise>(0xFF20);

    sample_timer_val            = CLOCK_SPEED / SAMPLE_RATE;
    frame_sequence_timer_val    = CLOCK_SPEED / 512;

    sample_timer                = sample_timer_val;
    frame_sequence_timer        = frame_sequence_timer_val;

    outRightChannel = std::make_unique<std::ofstream>("outRightChannel.pcm", std::ios::binary);
    outLeftChannel = std::make_unique<std::ofstream>("outLeftChannel.pcm", std::ios::binary);

    sample_buffer.resize(SAMPLE_BUFFER_SIZE);

    //return;

    initSDLAudio();
}

APU::~APU()
{
    outRightChannel->close();
    outLeftChannel->close();

    SDL_CloseAudioDevice(audio_device_id);
}

void APU::initSDLAudio()
{
    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        logger->error("SDL_Init(SDL_INIT_AUDIO) failed: {0:s}", SDL_GetError());
    }

    // SDL Configuration
    SDL_AudioSpec desiredSpec;
    desiredSpec.callback = NULL;
#ifndef USE_FLOAT
    desiredSpec.format = AUDIO_U8;
#else
    desiredSpec.format      = AUDIO_F32SYS;
#endif
    desiredSpec.freq = 44100;
    desiredSpec.channels = 2;
    desiredSpec.samples = SAMPLE_BUFFER_SIZE;
    desiredSpec.userdata = this;

    SDL_AudioSpec obtainedSpec;

    // Open SDL Audio instance
    audio_device_id = SDL_OpenAudioDevice(NULL,
        0,
        &desiredSpec,
        &obtainedSpec,
        0);

    if (audio_device_id == 0)
    {
        logger->error("Failed to open audio: {0:s}", SDL_GetError());
    }
    
    if (obtainedSpec.format != desiredSpec.format)
    {
        logger->error("Failed to get audio format requested");
    }

    // Get 'silence' value/byte
    sdl_silence_val = obtainedSpec.silence;

    // Start playing audio
    SDL_PauseAudioDevice(audio_device_id, 0);

    // Clear current audio queue
    SDL_ClearQueuedAudio(audio_device_id);
}

void APU::setByte(const uint16_t & addr, const uint8_t & val)
{
    if (addr >= 0xFF10 && addr <= 0xFF14)
    {   // Channel 1 - Square 1
        sound_channel_1->setByte(addr, val);
    }
    else if (addr >= 0xFF16 && addr <= 0xFF19)
    {   // Channel 2 - Square 2
        sound_channel_2->setByte(addr, val);
    }
    else if (addr >= 0xFF1A && addr <= 0xFF1E)
    {   // Channel 3 - Wave
        sound_channel_3->setByte(addr, val);
    }
    else if (addr >= 0xFF20 && addr <= 0xFF23)
    {   // Channel 4 - Noise
        sound_channel_4->setByte(addr, val);
    }
    else if (addr >= 0xFF30 && addr <= 0xFF3F)
    {   // Wave Pattern RAM
        sound_channel_3->setByte(addr, val);
    }

    switch (addr)
    {
    case 0xFF24:
        channel_control     = val;
        left_out_enabled    = val & BIT7;
        right_out_enabled   = val & BIT3;
        left_volume         = (val & 0x70) >> 4;
        right_volume        = val & 0x07;

        left_volume_use     = (128 * left_volume) / 7;
        right_volume_use    = (128 * right_volume) / 7;
        break;
    case 0xFF25: selection_of_sound_output  = val; break;
    case 0xFF26:

        if (sound_on && (val & BIT7) == 0)
        {   // Disabling sound, reset APU

        }
        else if (!sound_on && (val & BIT7))
        {   // Enabling sound
            frame_sequence_step = 0;

            sound_channel_1->duty_pos = 0;
            sound_channel_2->duty_pos = 0;
        }

        sound_on &= 0x7F;
        sound_on |= val & BIT7; // Only BIT7 is writable
        break;
    }
}

uint8_t APU::readByte(const uint16_t & addr)
{
    if (addr >= 0xFF10 && addr <= 0xFF14)
    {
        return sound_channel_1->readByte(addr);
    }
    else if (addr >= 0xFF16 && addr <= 0xFF19)
    {
        return sound_channel_2->readByte(addr);
    }
    else if (addr >= 0xFF1A && addr <= 0xFF1E)
    {
        return sound_channel_3->readByte(addr);
    }
    else if (addr >= 0xFF20 && addr <= 0xFF23)
    {
        return sound_channel_4->readByte(addr);
    }
    else if (addr >= 0xFF30 && addr <= 0xFF3F)
    {
        return sound_channel_3->readByte(addr);
    }

    switch (addr)
    {
    case 0xFF24: return channel_control;
    case 0xFF25: return selection_of_sound_output;
    case 0xFF26:
        return (sound_on & 0x80)
            | static_cast<uint8_t>(sound_channel_1->is_enabled)
            | static_cast<uint8_t>((sound_channel_2->is_enabled)) << 1
            | static_cast<uint8_t>((sound_channel_3->is_enabled)) << 2
            | static_cast<uint8_t>((sound_channel_4->is_enabled)) << 3;
    }

    return 0xFF;
}

void APU::run(const uint64_t & cpuTicks)
{
    //return;

    uint64_t diff = cpuTicks - prev_cpu_ticks;
    prev_cpu_ticks = cpuTicks;

    while (diff > 0)
    {
        // Tick frame sequencer
        if (frame_sequence_timer > 0)
        {
            frame_sequence_timer--;
        }

        if (frame_sequence_timer == 0)
        {
            switch (frame_sequence_step)
            {
            case 0:
            case 4:
                sound_channel_1->tickLengthCounter();
                sound_channel_2->tickLengthCounter();
                sound_channel_3->tickLengthCounter();
                //sound_channel_4->tickLengthCounter();
                break;
            case 2:
            case 6:
                sound_channel_1->tickSweep();
                sound_channel_1->tickLengthCounter();
                sound_channel_2->tickLengthCounter();
                sound_channel_3->tickLengthCounter();
                //sound_channel_4->tickLengthCounter();
                break;
            case 7:
                sound_channel_1->tickVolumeEnvelope();
                sound_channel_2->tickVolumeEnvelope();
                //sound_channel_4->tickVolumeEnvelope();
                break;
            }

            frame_sequence_step++;
            frame_sequence_step &= 7; // Sequence can only be 0..7

            // Reset frame_sequence_timer
            frame_sequence_timer = frame_sequence_timer_val;
        }

        // Tick sound channels
        sound_channel_1->tick();
        sound_channel_2->tick();
        sound_channel_3->tick();
        //sound_channel_4->tick();

        // Tick sample_timer
        if (sample_timer > 0)
        {
            sample_timer--;
        }

        if (sample_timer == 0)
        {   // Get samples, send sample to audio out buffer
            Sample sample;

            if ((sound_on & BIT7))
            {   // Audio is enabled
                // Get samples
#ifndef USE_FLOAT
                uint8_t channel_1_sample = sound_channel_1->output_volume;
                uint8_t channel_2_sample = sound_channel_2->output_volume;
                uint8_t channel_3_sample = sound_channel_3->output_volume;
#else
                float channel_1_sample = ((float)sound_channel_1->output_volume) / 100.0;
                float channel_2_sample = ((float)sound_channel_2->output_volume) / 100.0;
                float channel_3_sample = ((float)sound_channel_3->output_volume) / 100.0;
#endif

                // Apply samples to left and/or right out channels
#ifndef USE_FLOAT
                sendChannelOutputToSample(sample, channel_1_sample, 1);
                sendChannelOutputToSample(sample, channel_2_sample, 2);
                //sendChannelOutputToSample(sample, channel_3_sample, 3);
#else
                sendChannelOutputToSampleFloat(sample, channel_1_sample, 1);
                sendChannelOutputToSampleFloat(sample, channel_2_sample, 2);
                sendChannelOutputToSampleFloat(sample, channel_3_sample, 3);
#endif
            } // end if(sound_on)

            // Add current sample to sample buffer
            sample_buffer[sample_buffer_counter++] = sample;
            samplesPerFrame++;

            // Check if sample buffer is full
            if (sample_buffer_counter >= SAMPLE_BUFFER_SIZE)
            {
#ifdef USE_AUDIO_TIMING
                // Drain audio buffer (?)
                uint32_t queuedAudioSize = SDL_GetQueuedAudioSize(audio_device_id);

#ifndef USE_FLOAT
                while (queuedAudioSize > SAMPLE_BUFFER_MEM_SIZE)
#else
                while (queuedAudioSize >= SAMPLE_BUFFER_MEM_SIZE_FLOAT)
                //while (queuedAudioSize > SAMPLE_BUFFER_SIZE * sizeof(float))
#endif
                {
                    //logger->info("queuedAudioSize: {0:d}", queuedAudioSize);
                    //SDL_Delay(1);
                    //std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                    queuedAudioSize = SDL_GetQueuedAudioSize(audio_device_id);
                }
#endif // USE_AUDIO_TIMING

                //logger->info("Pushing sample");

                // Push sample_buffer to SDL
#ifndef USE_FLOAT
                int ret = SDL_QueueAudio(audio_device_id, reinterpret_cast<uint8_t *>(sample_buffer.data()), SAMPLE_BUFFER_MEM_SIZE);
#else
                int ret = SDL_QueueAudio(audio_device_id, reinterpret_cast<float *>(sample_buffer.data()), SAMPLE_BUFFER_MEM_SIZE_FLOAT);
#endif

                if (ret != 0)
                {
                    logger->error("ret: {0:d}", ret);
                }

#ifdef WRITE_AUDIO_OUT
#ifndef USE_FLOAT
                outLeftChannel->write(reinterpret_cast<char *>(sample_buffer.data()), SAMPLE_BUFFER_MEM_SIZE);
#else
                outLeftChannel->write(reinterpret_cast<char *>(sample_buffer.data()), SAMPLE_BUFFER_MEM_SIZE_FLOAT);
#endif // USE_FLOAT
#endif // WRITE_AUDIO_OUT

                // Clear sample_buffer
                sample_buffer_counter = 0;
                sample_buffer.clear();
                sample_buffer.resize(SAMPLE_BUFFER_SIZE);
            }

            // Reset sample_timer
            sample_timer = sample_timer_val;

        } // end if(sample_timer)

        diff--;
    }
}

bool APU::isSoundOutLeft(uint8_t sound_number)
{
    switch (sound_number)
    {
    case 4: return selection_of_sound_output & BIT7;
    case 3: return selection_of_sound_output & BIT6;
    case 2: return selection_of_sound_output & BIT5;
    case 1: return selection_of_sound_output & BIT4;
    default: return false;
    }
}

bool APU::isSoundOutRight(uint8_t sound_number)
{
    switch (sound_number)
    {
    case 4: return selection_of_sound_output & BIT3;
    case 3: return selection_of_sound_output & BIT2;
    case 2: return selection_of_sound_output & BIT1;
    case 1: return selection_of_sound_output & BIT0;
    default: return false;
    }
}

#ifndef USE_FLOAT
void APU::sendChannelOutputToSample(Sample & sample, const uint8_t & audio, const uint8_t & channelNum)
{
    if (isSoundOutLeft(channelNum))
    {
        //sample.left += audio;
       SDL_MixAudioFormat(&sample.left, &audio, AUDIO_U8, 1, left_volume_use);
        //sample.left = mixAudio(sample.left, audio);
    }

    if (isSoundOutRight(channelNum))
    {
        //sample.right += audio;
        SDL_MixAudioFormat(&sample.right, &audio, AUDIO_U8, 1, right_volume_use);
        //sample.right = mixAudio(sample.right, audio);
    }
}
#else
void APU::sendChannelOutputToSampleFloat(Sample & sample, float & audio, const uint8_t & channelNum)
{
    if (isSoundOutLeft(channelNum))
    {
        SDL_MixAudioFormat((uint8_t *)&sample.left, 
            (uint8_t *)&audio,
            AUDIO_F32SYS,
            sizeof(float),
            left_volume_use);
    }

    if (isSoundOutRight(channelNum))
    {
        SDL_MixAudioFormat((uint8_t *)&sample.right,
            (uint8_t *)&audio,
            AUDIO_F32SYS,
            sizeof(float),
            right_volume_use);
    }
}
#endif

// http://www.vttoth.com/CMS/index.php/technical-notes/68
uint8_t APU::mixAudio(const uint8_t & audio1, const uint8_t & audio2)
{
    uint8_t ret = 0;

    if (audio1 < 128 && audio2 < 128)
    {
        ret = (static_cast<uint16_t>(audio1) * static_cast<uint16_t>(audio2)) / 128;
    }
    else
    {
        ret = (2 * (static_cast<uint16_t>(audio1) + static_cast<uint16_t>(audio2))) -
            ((static_cast<uint16_t>(audio1) * static_cast<uint16_t>(audio2)) / 128) -
            256;
    }
    return ret;
}

void APU::logSamples()
{
    std::string stringToLog = "";

    for (auto & sample : sample_buffer)
    {
        stringToLog += sample.getSampleStr();
    }

    logger->trace(stringToLog);
}

void APU::initCGB()
{
    sample_timer_val            = CLOCK_SPEED_GBC_MAX / SAMPLE_RATE;
    frame_sequence_timer_val    = CLOCK_SPEED_GBC_MAX / 512;

    sample_timer                = sample_timer_val;
    frame_sequence_timer        = frame_sequence_timer_val;
}