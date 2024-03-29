#include <APU.h>
#include <GBCEmulator.h>
#include <CPU.h>
#include <Joypad.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <chrono>

APU::APU(std::shared_ptr<spdlog::sinks::rotating_file_sink_st> logger_sink, std::shared_ptr<spdlog::logger> _logger)
    : SAMPLE_BUFFER_SIZE(1470)//1470
    , SAMPLE_OUTPUT_CHANNEL_SIZE(2)
    , SAMPLE_BUFFER_MEM_SIZE(SAMPLE_BUFFER_SIZE * SAMPLE_OUTPUT_CHANNEL_SIZE)
    , SAMPLE_BUFFER_MEM_SIZE_FLOAT(SAMPLE_BUFFER_MEM_SIZE * sizeof(float))
    , sample_timer_val(CLOCK_SPEED / SAMPLE_RATE)
    , frame_sequence_timer_val(CLOCK_SPEED / 512)
{
    logger = _logger;
    sound_channel_1 = std::make_unique<AudioSquare>(0xFF10, std::make_shared<spdlog::logger>("APU.Channel.1", logger_sink));
    sound_channel_2 = std::make_unique<AudioSquare>(0xFF15, std::make_shared<spdlog::logger>("APU.Channel.2", logger_sink));
    sound_channel_3 = std::make_unique<AudioWave>(0xFF1A,   std::make_shared<spdlog::logger>("APU.Channel.3", logger_sink));
    sound_channel_4 = std::make_unique<AudioNoise>(0xFF20,  std::make_shared<spdlog::logger>("APU.Channel.4", logger_sink));
    frame_sequence_step     = 0;
    channel_control         = 0;
    selection_of_sound_output = 0;
    sound_on                = 0;
    left_volume             = 0;
    right_volume            = 0;
    left_volume_use         = 0;
    right_volume_use        = 0;
    sample_buffer_counter   = 0;
    samplesPerFrame         = 0;
    left_out_enabled        = false;
    right_out_enabled       = false;
    double_speed_mode       = false;
    send_samples_to_debugger = false;
    initialized             = false;
    curr_sample_buffer      = 0;

    sample_timer                = sample_timer_val;
    frame_sequence_timer        = frame_sequence_timer_val;

#ifdef WRITE_AUDIO_OUT
    audioFileOut = std::make_unique<std::ofstream>("audioOut.pcm", std::ios::binary);
#endif // WRITE_AUDIO_OUT

    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        logger->error("SDL_Init(SDL_INIT_AUDIO) failed: {0:s}", SDL_GetError());
    }

    initSDLAudio();

    // Initialize double sample buffer
    double_sample_buffer[0].resize(SAMPLE_BUFFER_SIZE);
    double_sample_buffer[1].resize(SAMPLE_BUFFER_SIZE);
}

APU::~APU()
{
#ifdef WRITE_AUDIO_OUT
    audioFileOut->close();
#endif // WRITE_AUDIO_OUT

    if (initialized)
    {
        SDL_CloseAudioDevice(audio_device_id);
    }
}

APU& APU::operator=(const APU& rhs)
{   // Copy APU from rhs
    frame_sequence_step         = rhs.frame_sequence_step;
    channel_control             = rhs.channel_control;
    selection_of_sound_output   = rhs.selection_of_sound_output;
    sound_on                    = rhs.sound_on;
    left_volume                 = rhs.left_volume;
    right_volume                = rhs.right_volume;
    left_volume_use             = rhs.left_volume_use;
    right_volume_use            = rhs.right_volume_use;
    sample_buffer_counter       = rhs.sample_buffer_counter;
    samplesPerFrame             = rhs.samplesPerFrame;
    left_out_enabled            = rhs.left_out_enabled;
    right_out_enabled           = rhs.right_out_enabled;
    double_speed_mode           = rhs.double_speed_mode;
    send_samples_to_debugger    = rhs.send_samples_to_debugger;
    audio_device_id             = rhs.audio_device_id;

    *sound_channel_1.get() = *rhs.sound_channel_1.get();
    *sound_channel_2.get() = *rhs.sound_channel_2.get();
    *sound_channel_3.get() = *rhs.sound_channel_3.get();
    *sound_channel_4.get() = *rhs.sound_channel_4.get();

    sample_timer            = rhs.sample_timer;
    frame_sequence_timer    = rhs.frame_sequence_timer;
    
    double_sample_buffer[0] = rhs.double_sample_buffer[0];
    double_sample_buffer[1] = rhs.double_sample_buffer[1];
    sample_buffer_counter   = rhs.sample_buffer_counter;

    return *this;
}

void APU::initSDLAudio()
{
    // SDL Configuration
    desired_spec.callback = NULL;
#ifndef USE_FLOAT
    desired_spec.format  = AUDIO_U8;
#else
    desired_spec.format  = AUDIO_F32SYS;
#endif
    desired_spec.freq = SAMPLE_RATE;
    desired_spec.channels = 2;
    //desired_spec.samples = SAMPLE_BUFFER_SIZE * 2;
    desired_spec.samples = SAMPLE_BUFFER_SIZE;
    desired_spec.userdata = this;

    // Open SDL Audio instance
    audio_device_id = SDL_OpenAudioDevice(NULL,
        0,
        &desired_spec,
        &obtained_spec,
        SDL_AUDIO_ALLOW_ANY_CHANGE);

    if (audio_device_id == 0)
    {
        logger->error("Failed to open audio: {0:s}", SDL_GetError());
        return;
    }
    
    if (obtained_spec.format != desired_spec.format)
    {
        logger->error("Failed to get audio format requested, from: {} to: {}",
            desired_spec.samples,
            obtained_spec.samples);
    }

    // Get 'silence' value/byte
    sdl_silence_val = obtained_spec.silence;

    // Start playing audio
    SDL_PauseAudioDevice(audio_device_id, 0);

    // Clear current audio queue
    SDL_ClearQueuedAudio(audio_device_id);

    initialized = true;
}

void APU::setByte(const uint16_t & addr, const uint8_t & val)
{
    if (sound_on == false &&
        addr < 0xFF26 &&
        addr != 0xFF20) // NR41 can still be written to when off
    {
        logger->info("Tried to write to addr: 0x{0:x}, val 0x{1:x} but APU sound is disabled",
            addr,
            val);
        return;
    }

    logger->debug("Writing to addr: 0x{0:x}, val: 0x{1:x}",
        addr,
        val);

    if (addr >= 0xFF10 && addr <= 0xFF14)
    {   // Channel 1 - Square 1 - NR10-NR14
        sound_channel_1->setByte(addr, val);
    }
    else if (addr >= 0xFF16 && addr <= 0xFF19)
    {   // Channel 2 - Square 2 - NR21-NR24
        sound_channel_2->setByte(addr, val);
    }
    else if (addr >= 0xFF1A && addr <= 0xFF1E)
    {   // Channel 3 - Wave - NR30-NR34
        sound_channel_3->setByte(addr, val);
    }
    else if (addr >= 0xFF20 && addr <= 0xFF23)
    {   // Channel 4 - Noise - NR41-NR44
        sound_channel_4->setByte(addr, val);
    }
    else if (addr >= 0xFF30 && addr <= 0xFF3F)
    {   // Wave Pattern RAM
        sound_channel_3->setByte(addr, val);
    }

    switch (addr)
    {
    case 0xFF24:    // NR50
        channel_control     = val;
        left_out_enabled    = val & BIT7;
        right_out_enabled   = val & BIT3;
        left_volume         = (val & 0x70) >> 4;
        right_volume        = val & 0x07;

        left_volume_use     = (128 * (left_volume + 1)) / 7;
        right_volume_use    = (128 * (right_volume + 1)) / 7;
        break;
    case 0xFF25:    // NR51
        selection_of_sound_output  = val;
        break;
    case 0xFF26:    // NR52
        if (sound_on && (val & BIT7) == 0)
        {   // Disabling sound, reset APU
            logger->info("Turning APU off, zeroing out 0xFF10-0xFF25");
            // Write 0s to APU registers NR10-NR51 (0xFF10-0xFF25)
            for (uint16_t i = 0xFF10; i < 0xFF26; i++)
            {
                setByte(i, 0);
            }
            sound_channel_1->is_enabled = false;
            sound_channel_2->is_enabled = false;
            sound_channel_3->is_enabled = false;
            sound_channel_4->is_enabled = false;
        }
        else if (!sound_on && (val & BIT7))
        {   // Enabling sound
            logger->info("Turning APU on, resetting APU");
            reset();
        }

        sound_on = val & BIT7; // Only BIT7 is writable
        break;
    }
}

uint8_t APU::readByte(const uint16_t & addr) const
{
    uint8_t ret = 0xFF;

    if (addr >= 0xFF10 && addr <= 0xFF14)
    {
        ret = sound_channel_1->readByte(addr);
    }
    else if (addr >= 0xFF16 && addr <= 0xFF19)
    {
        ret = sound_channel_2->readByte(addr);
    }
    else if (addr >= 0xFF1A && addr <= 0xFF1E)
    {
        ret = sound_channel_3->readByte(addr);
    }
    else if (addr >= 0xFF20 && addr <= 0xFF23)
    {
        ret = sound_channel_4->readByte(addr);
    }
    else if (addr >= 0xFF30 && addr <= 0xFF3F)
    {
        ret = sound_channel_3->readByte(addr);
    }

    switch (addr)
    {
    case 0xFF24:    // NR50
        ret = channel_control;
        break;
    case 0xFF25:    // NR51
        ret = selection_of_sound_output;
        break;
    case 0xFF26:    // NR52
        ret = static_cast<uint8_t>(sound_on) << 7
            | 0x70  // Unused bits are 1s
            | static_cast<uint8_t>(sound_channel_1->isRunning())
            | static_cast<uint8_t>((sound_channel_2->isRunning()) << 1)
            | static_cast<uint8_t>((sound_channel_3->isRunning()) << 2)
            | static_cast<uint8_t>((sound_channel_4->isRunning()) << 3);
         break;
    }

    logger->debug("Reading addr: 0x{0:x}, val: 0x{1:x}",
        addr,
        ret);

    return ret;
}

void APU::reset()
{
    frame_sequence_timer    = frame_sequence_timer_val;
    frame_sequence_step     = 0;
    sample_timer            = sample_timer_val;
    sample_buffer_counter   = 0;
    samplesPerFrame         = 0;

    // Reset Wave RAM
    for (uint16_t i = 0xFF30; i < 0xFF3F; i++)
    {
        sound_channel_3->setByte(i, 0);
    }

    sound_channel_1->reset();
    sound_channel_2->reset();
    sound_channel_3->reset();
    sound_channel_4->reset();

    SDL_ClearQueuedAudio(audio_device_id);
}

void APU::run(const uint8_t & cpuTickDiff)
{
    uint8_t diff = cpuTickDiff;

    while (diff > 0)
    {
        diff--;

        // Tick frame sequencer
        if (frame_sequence_timer > 0)
        {
            frame_sequence_timer--;
            //logger->debug("frame_sequence_timer--: 0x{0:x}", frame_sequence_timer);
        }

        if (frame_sequence_timer == 0)
        {
            logger->trace("frame_sequnce_timer == 0, frame_sequence_step: 0x{0:x}",
                frame_sequence_step);
            switch (frame_sequence_step)
            {
            case 0:
            case 4:
                sound_channel_1->tickLengthCounter();
                sound_channel_2->tickLengthCounter();
                sound_channel_3->tickLengthCounter();
                sound_channel_4->tickLengthCounter();
                break;
            case 2:
            case 6:
                sound_channel_1->tickLengthCounter();
                sound_channel_2->tickLengthCounter();
                sound_channel_3->tickLengthCounter();
                sound_channel_4->tickLengthCounter();
                sound_channel_1->tickSweep();
                break;
            case 7:
                sound_channel_1->tickVolumeEnvelope();
                sound_channel_2->tickVolumeEnvelope();
                sound_channel_4->tickVolumeEnvelope();
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
        sound_channel_4->tick();

        // Tick sample_timer
        if (sample_timer > 0)
        {
            sample_timer--;
        }

        if (sample_timer == 0)
        {   // Get samples, send sample to audio out buffer
            Sample sample;

            if (sound_on)
            {   // Audio is enabled
                // Get samples
#ifndef USE_FLOAT
                uint8_t channel_1_sample = sound_channel_1->output_volume;
                uint8_t channel_2_sample = sound_channel_2->output_volume;
                uint8_t channel_3_sample = sound_channel_3->output_volume;
                uint8_t channel_4_sample = sound_channel_4->output_volume;
#else
                float channel_1_sample = (sound_channel_1->output_volume) ? ((float)sound_channel_1->output_volume) / 60.0f : 0.0f;   // 30.0f = 0x0F * 2.0f; 0x0F = MAX_CHANNEL_VOL 
                float channel_2_sample = (sound_channel_2->output_volume) ? ((float)sound_channel_2->output_volume) / 60.0f : 0.0f;   // 30.0f = 0x0F * 2.0f; 0x0F = MAX_CHANNEL_VOL 
                float channel_3_sample = (sound_channel_3->output_volume) ? ((float)sound_channel_3->output_volume) / 60.0f : 0.0f;   // 30.0f = 0x0F * 2.0f; 0x0F = MAX_CHANNEL_VOL 
                float channel_4_sample = (sound_channel_4->output_volume) ? ((float)sound_channel_4->output_volume) / 60.0f : 0.0f;   // 30.0f = 0x0F * 2.0f; 0x0F = MAX_CHANNEL_VOL 
#endif

                // Apply samples to left and/or right out channels
#ifndef USE_FLOAT
                sendChannelOutputToSample(sample, channel_1_sample, 1);
                sendChannelOutputToSample(sample, channel_2_sample, 2);
                sendChannelOutputToSample(sample, channel_3_sample, 3);
                sendChannelOutputToSample(sample, channel_4_sample, 4);
#else
                sendChannelOutputToSampleFloat(sample, channel_1_sample, 1);
                sendChannelOutputToSampleFloat(sample, channel_2_sample, 2);
                sendChannelOutputToSampleFloat(sample, channel_3_sample, 3);
                sendChannelOutputToSampleFloat(sample, channel_4_sample, 4);

                if (send_samples_to_debugger)
                {
                    sendSampleUpdate(channel_1_sample, 0);
                    sendSampleUpdate(channel_2_sample, 1);
                    sendSampleUpdate(channel_3_sample, 2);
                    sendSampleUpdate(channel_4_sample, 3);
                }
#endif
            } // end if(sound_on)

            // Add current sample to sample buffer
            std::vector<Sample>& sample_buffer = double_sample_buffer[curr_sample_buffer];
            sample_buffer[sample_buffer_counter++] = sample;
            samplesPerFrame++;

            // Check if sample buffer is full
            if (sample_buffer_counter >= SAMPLE_BUFFER_SIZE)
            {   // Force write out audio samples
                writeSamplesOutAsync(audio_device_id);
            }

            //} // end if(sound_on)

            // Reset sample_timer
            sample_timer = sample_timer_val;

        } // end if(sample_timer)

    } // end while(diff > 0)
}

void APU::writeSamplesOut(const uint32_t& audio_device, const std::vector<Sample>& samples, const uint16_t num_samples)
{
    if (!initialized)
    {
        return;
    }

    logger->trace("Pushing sample of size: {}", samples.size());

    // Push sample_buffer to SDL
#ifndef USE_FLOAT
    prev_sample_size = num_samples * 2 * sizeof(uint8_t);
    const int ret = SDL_QueueAudio(audio_device_id, reinterpret_cast<const uint8_t*>(samples.data()), prev_sample_size);
#else
    prev_sample_size = num_samples * 2 * sizeof(float);
    const int ret = SDL_QueueAudio(audio_device_id, reinterpret_cast<const float*>(samples.data()), prev_sample_size);
#endif // USE_FLOAT

    rolling_avg_sample_size.Push(prev_sample_size);

    if (ret != 0)
    {
        logger->error("SDL_QueueAudio returned {}", ret);
    }

#ifdef WRITE_AUDIO_OUT
    audioFileOut->write(reinterpret_cast<const char*>(samples.data()), prev_sample_size);
#endif // WRITE_AUDIO_OUT
}

void APU::writeSamplesOutAsync(const uint32_t& audio_device)
{
    const std::vector<Sample>& sampleBuffer =
        double_sample_buffer[curr_sample_buffer];

    const uint16_t sampleBufferSize = sample_buffer_counter;

    // Change active sample buffer
    curr_sample_buffer = !curr_sample_buffer;

    // Clear active sample_buffer for immediate use
    clearCurrentAudioBuffer();

    writeSamplesOut(audio_device, sampleBuffer,
      sampleBufferSize);
}

bool APU::isSoundOutLeft(uint8_t sound_number) const
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

bool APU::isSoundOutRight(uint8_t sound_number) const
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
void APU::sendChannelOutputToSample(Sample & sample, const uint8_t & audio, const uint8_t & channelNum) const
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
void APU::sendChannelOutputToSampleFloat(Sample & sample, const float & audio, const uint8_t & channelNum) const
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

    for (auto & sample : double_sample_buffer[curr_sample_buffer])
    {
        stringToLog += sample.getSampleStr();
    }

    logger->trace(stringToLog);
}

void APU::initCGB()
{
    double_speed_mode = true;

    sample_timer                = sample_timer_val;
    frame_sequence_timer        = frame_sequence_timer_val;

    // Clear sample_buffer
    clearCurrentAudioBuffer();
}

void APU::clearCurrentAudioBuffer()
{
    logger->trace("Clearing sample buffer: {}", curr_sample_buffer);

    // Get current sample buffer
    std::vector<Sample>& sample_buffer =
        double_sample_buffer[curr_sample_buffer];

    // Clear it
    sample_buffer_counter = 0;
    if (sample_buffer.size() != SAMPLE_BUFFER_SIZE)
    {   // Resize buffer if needed
        sample_buffer.clear();
        sample_buffer.resize(SAMPLE_BUFFER_SIZE);
    }
//#ifdef USE_FLOAT
//    std::fill(sample_buffer.begin(), sample_buffer.end(), Sample{ 0.0f, 0.0f });
//#else
//    std::fill(sample_buffer.begin(), sample_buffer.end(), Sample{ 0, 0 });
//#endif // USE_FLOAT
}

void APU::setChannelLogLevel(spdlog::level::level_enum level)
{
    sound_channel_1->logger->set_level(level);
    sound_channel_2->logger->set_level(level);
    sound_channel_3->logger->set_level(level);
    sound_channel_4->logger->set_level(level);
}

void APU::sleepUntilBufferIsEmpty(const std::chrono::duration<double>& frame_start_time)
{
    int microElapsedInt = 0;

    // Drain audio buffer (?)
    uint32_t queuedAudioSize = SDL_GetQueuedAudioSize(audio_device_id);
    const uint32_t queuedAudioSizeOrig = queuedAudioSize;
    const uint32_t singleFrameAudioBufferSize = samplesPerFrame * 2 * sizeof(float);
    const size_t rollingAvgSampleSize = rolling_avg_sample_size.GetRollingAvg() * 2 * sizeof(float);

#ifndef USE_FLOAT
    while (queuedAudioSize > SAMPLE_BUFFER_MEM_SIZE)
#else
    //while (queuedAudioSize >= SAMPLE_BUFFER_MEM_SIZE_FLOAT)
    //while (queuedAudioSize >= singleFrameAudioBufferSize)
    //while (queuedAudioSize >= obtained_spec.size)
    while (queuedAudioSize > rollingAvgSampleSize)
#endif
    {
        logger->trace("queuedAudioSize: {}, rollingAvgSampleSize: {}",
            queuedAudioSize,
            rollingAvgSampleSize);

        // Check if time spent sleeping is greater than 1 frame time (16.667 ms)
        //const auto currTime = std::chrono::system_clock::now().time_since_epoch();
        //const auto microsecondsElapsed = std::chrono::duration_cast<std::chrono::microseconds>(currTime - frame_start_time);
        //microElapsedInt = microsecondsElapsed.count();
        //if (microElapsedInt >= MICROSEC_PER_FRAME - 100)
        //{
        //    break;
        //}

        // Sleep for 1 millisecond
        SDL_Delay(1);   // std::this_thread::sleep_for() causes audio delay on Linux
        //std::this_thread::sleep_for(std::chrono::microseconds(200));
        queuedAudioSize = SDL_GetQueuedAudioSize(audio_device_id);
    }

    logger->trace("Slept for {} milliseconds, buffer size diff: {}, buffer size start: {}, buffer size end: {}",
        microElapsedInt / 1000.0,
        queuedAudioSizeOrig - queuedAudioSize,
        queuedAudioSizeOrig,
        queuedAudioSize);

    samplesPerFrame = 0;
}

void APU::setSampleUpdateMethod(std::function<void(float, int)> function)
{
    sendSampleUpdate = function;
}

void APU::sendSamplesToDebugger(bool b)
{
    send_samples_to_debugger = b;
}