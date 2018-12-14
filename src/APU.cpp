#include "APU.h"
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

    initSDLAudio();

    // Start playing audio
    SDL_PauseAudio(0);
}

APU::~APU()
{
    outRightChannel->close();
    outLeftChannel->close();
}

void APU::initSDLAudio()
{
    SDL_Init(SDL_INIT_AUDIO);

    // SDL Configuration
    SDL_AudioSpec desiredSpec;
    desiredSpec.callback = NULL;
    //desiredSpec.format = AUDIO_U8;
    desiredSpec.format      = AUDIO_F32SYS;
    desiredSpec.freq = 44100;
    desiredSpec.channels = 2;
    desiredSpec.samples = SAMPLE_BUFFER_SIZE;
    desiredSpec.userdata = this;

    SDL_AudioSpec obtainedSpec;

    // Open SDL Audio instance
    //SDL_OpenAudio(&desiredSpec, &obtainedSpec);
    audio_device_id = SDL_OpenAudioDevice(NULL,
        0,
        &desiredSpec,
        &obtainedSpec,
        0);

    // Get 'silence' value/byte
    sdl_silence_val = obtainedSpec.silence;
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
    uint64_t diff = cpuTicks - prev_cpu_ticks;

    prev_cpu_ticks = cpuTicks;

    while (diff > 0)
    {
        // Tick sound channels
        sound_channel_1->tick();
        sound_channel_2->tick();

        // Tick frame sequencer
        if (frame_sequence_timer > 0)
        {
            frame_sequence_timer--;
        }

        if (frame_sequence_timer == 0)
        {
            if (frame_sequence_step % 2 == 0)
            {   // Tick Length counter, is updated every even step (0, 2, 4, 6)
                sound_channel_1->tickLengthCounter();
                sound_channel_2->tickLengthCounter();
            }

            if (frame_sequence_step == 2 ||
                frame_sequence_step == 6)
            {   // Tick Sweep
                //sound_channel_1->tickSweep();
            }

            if (frame_sequence_step == 7)
            {   // Tick Volume Envelope
                //sound_channel_1->tickVolumeEnvelope();
                //sound_channel_2->tickVolumeEnvelope();
                //sound_channel_4->tickVolumeEnvelope();
            }

            frame_sequence_step++;
            frame_sequence_step &= 7; // Sequence can only be 0..7

            // Reset frame_sequence_timer
            frame_sequence_timer = frame_sequence_timer_val;
        }

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
                const uint8_t & channel_1_sample = sound_channel_1->curr_sample;
                const uint8_t & channel_2_sample = sound_channel_2->curr_sample;

                //float left = 0;
                //float right = 0;

                // Apply samples to left and/or right out channels
                if (isSoundOutLeft(1))
                {
                    sample.left += channel_1_sample;
                    //sample.right = ((float)channel_1_sample) * 15;
                    //SDL_MixAudioFormat((uint8_t*)&sample.left, (uint8_t*)&sample.right, AUDIO_F32SYS, sizeof(float), SDL_MIX_MAXVOLUME / 2);
                }

                if (isSoundOutRight(1))
                {
                    sample.right += channel_1_sample;
                    //sample.right = ((float)channel_1_sample) * 15;
                    //SDL_MixAudioFormat((uint8_t*)&sample.left, (uint8_t*)&sample.right, AUDIO_F32SYS, sizeof(float), SDL_MIX_MAXVOLUME / 2);
                }

                if (isSoundOutLeft(2))
                {
                    sample.left += channel_2_sample;
                    //sample.right = ((float)channel_1_sample) * 15;
                    //SDL_MixAudioFormat((uint8_t*)&sample.left, (uint8_t*)&sample.right, AUDIO_F32SYS, sizeof(float), SDL_MIX_MAXVOLUME / 2);
                }

                if (isSoundOutRight(2))
                {
                    sample.right += channel_2_sample;
                    //sample.right = ((float)channel_1_sample) * 15;
                    //SDL_MixAudioFormat((uint8_t*)&sample.left, (uint8_t*)&sample.right, AUDIO_F32SYS, sizeof(float), SDL_MIX_MAXVOLUME / 2);
                }
            } // end if(sound_on)

            // Multiply left and right samples by volume
            sample.left *= (left_volume + 1);
            sample.right *= (right_volume + 1);

            //if (sample.left == 0)
            //{
            //    sample.left = sdl_silence_val;
            //}

            //if (sample.right == 0)
            //{
            //    sample.right = sdl_silence_val;
            //}

            // Convert 4-bit unsigned audio to 8-bit unsigned audio
            sample.left  = (sample.left << 4) | sample.left;
            sample.right = (sample.right << 4) | sample.right;

            //sample.left     /= 100.0f;
            //sample.right    /= 100.0f;

            // Add current sample to sample buffer
            //sample_buffer.push_back(std::move(sample));
            //sample_buffer.push_back(sample);
            sample_buffer[sample_buffer_counter++] = sample;

            samplesPerFrame++;

            // Check if sample buffer is full
            if (sample_buffer_counter >= SAMPLE_BUFFER_SIZE)
            {
                // Drain audio buffer (?)
                uint32_t queuedAudioSize = SDL_GetQueuedAudioSize(audio_device_id);
                while (queuedAudioSize > SAMPLE_BUFFER_UINT8_SIZE)
                //while (SDL_GetQueuedAudioSize(1) > SAMPLE_BUFFER_SIZE)
                //while (queuedAudioSize > SAMPLE_BUFFER_SIZE * sizeof(float))
                {
                    logger->info("queuedAudioSize: {0:d}", queuedAudioSize);
                    SDL_Delay(1);
                    queuedAudioSize = SDL_GetQueuedAudioSize(audio_device_id);
                }

                // Push sample_buffer to SDL
                int ret = SDL_QueueAudio(audio_device_id, reinterpret_cast<uint8_t *>(sample_buffer.data()), SAMPLE_BUFFER_UINT8_SIZE);
                //SDL_QueueAudio(audio_device_id, reinterpret_cast<uint8_t *>(sample_buffer.data()), SAMPLE_BUFFER_SIZE);
                //int ret = SDL_QueueAudio(audio_device_id, reinterpret_cast<float *>(sample_buffer.data()), SAMPLE_BUFFER_SIZE * sizeof(float));

                if (ret != 0)
                {
                    logger->error("ret: {0:d}", ret);
                }

                outLeftChannel->write(reinterpret_cast<char *>(sample_buffer.data()), SAMPLE_BUFFER_UINT8_SIZE);
                //outLeftChannel->write(reinterpret_cast<char *>(sample_buffer.data()), SAMPLE_BUFFER_SIZE * sizeof(float));

                // Clear sample_buffer
                //sample_buffer.clear();
                //sample_buffer.reserve(SAMPLE_BUFFER_SIZE);
                sample_buffer_counter = 0;
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