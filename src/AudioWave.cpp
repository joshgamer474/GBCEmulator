#include <AudioWave.h>
#include <CPU.h>
#include <Joypad.h>

AudioWave::AudioWave(const uint16_t & register_offset, std::shared_ptr<spdlog::logger> _logger)
    :   reg_offset(register_offset),
        logger(_logger)
{
    volume          = 0;
    output_volume   = 0;
    frequency_16    = 0;
    frequency       = 0;
    timer           = 64;
    period          = 0;
    nibble_pos      = 0;
    sound_length_data = 0.0f;
    restart_sound       = false;
    is_enabled          = false;
    channel_is_enabled  = false;
    stop_output_when_sound_length_ends = false;
}

AudioWave::~AudioWave()
{

}

AudioWave& AudioWave::operator=(const AudioWave& rhs)
{   // Copy from rhs
    volume              = rhs.volume;
    output_volume       = rhs.output_volume;
    frequency_16        = rhs.frequency_16;
    frequency           = rhs.frequency;
    timer               = rhs.timer;
    period              = rhs.period;
    nibble_pos          = rhs.nibble_pos;
    sound_length_data   = rhs.sound_length_data;
    restart_sound       = rhs.restart_sound;
    is_enabled          = rhs.is_enabled;
    channel_is_enabled  = rhs.channel_is_enabled;
    stop_output_when_sound_length_ends = rhs.stop_output_when_sound_length_ends;

    return *this;
}

void AudioWave::setByte(const uint16_t & addr, const uint8_t & val)
{
    switch (addr)
    {
    case 0xFF1A:    // NR30
        channel_is_enabled = val & BIT7;

        if (!channel_is_enabled)
        {   // Channel flag is disabled, completely disable the channel until reset()
            is_enabled = channel_is_enabled;
        }
        break;
    case 0xFF1B:    // NR31
        sound_length_data = 0x0100 - static_cast<uint16_t>(val & 0xFF);

        if (sound_length_data == 0x0100)
        {
            sound_length_data = 0xFF;
            logger->trace("Setting sound_length_data to 0xFF as it was 0x0100, but the register is supposed to be 1 byte");
        }

        break;
    case 0xFF1C:    // NR32
        volume = (val & 0x60) >> 5;
        break;
    case 0xFF1D:    // NR33
        frequency_16 &= 0xFF00;
        frequency_16 |= val;
        break;
    case 0xFF1E:    // NR34
        stop_output_when_sound_length_ends = val & BIT6;
        frequency_16 &= 0x00FF;
        frequency_16 |= (static_cast<uint16_t>(val) & 0x07) << 8;

        // Calculate frequency
        frequency = 131072 / (2048 - frequency_16);

        //if (restart_sound == false && (val & BIT7))
        //{
        //    reset();
        //}
        restart_sound = val & BIT7;
        if (restart_sound)
        {
            reset();
        }

        break;
    default:

        if (addr >= 0xFF30 && addr <= 0xFF3F)
        {
            //if (restart_sound)
            //{
            //    int j = 0;
            //}
            //else
            //{
                wave_pattern_RAM[addr - 0xFF30] = val;
            //}
        }
    }
}

uint8_t AudioWave::readByte(const uint16_t & addr) const
{
    uint8_t ret = 0xFF;

    switch (addr)
    {
    case 0xFF1A:    // NR30
        ret = static_cast<uint8_t>(channel_is_enabled) << 7;
        ret |= 0x7F;    // Unused bits are 1s
        break;
    case 0xFF1B:    // NR31
        ret = 0xFF;
        break;
    case 0xFF1C:    // NR32
        ret = (volume & 0x03) << 5;
        ret |= 0x9F;    // Unused bits are 1s
        break;
    case 0xFF1D:    // NR33
        ret = 0xFF; // Write only
        break;
    case 0xFF1E:    // NR34
        ret = static_cast<uint8_t>(stop_output_when_sound_length_ends) << 6;
        ret |= 0xBF;    // Unused bits are 1s
        break;
    default:

        if (addr >= 0xFF30 && addr <= 0xFF3F)
        {
            ret = wave_pattern_RAM[addr - 0xFF30];
        }
    }

    return ret;
}

void AudioWave::reset()
{
    is_enabled = true;

    // Calculate period
    period = (2048 - frequency_16) * 2;

    // Reload frequency period
    timer = period;

    // Reset nibble pos
    nibble_pos = 0;

    if (sound_length_data == 0)
    {
        //sound_length_data = 0x0100;
        sound_length_data = 0xFF;
    }
}

void AudioWave::tick()
{
    if (timer > 0)
    {
        timer--;
    }

    if (timer == 0)
    {
        // Calculate period
        period = (2048 - frequency_16) * 2;
        timer = period;

        // Increment nibble position
        nibble_pos++;
        nibble_pos &= 0x1F;
        
        if (is_enabled &&
            channel_is_enabled)
        {
            updateSample();
        }
    }
    

    // Update output
    if (is_enabled &&
        channel_is_enabled &&
        curr_sample != 0)
    {
        output_volume = curr_sample;
    }
    else
    {
        output_volume = 0;
    }
}

void AudioWave::tickLengthCounter()
{
    if (stop_output_when_sound_length_ends)
    {
        if (sound_length_data > 0)
        {
            sound_length_data--;
        }

        if (sound_length_data == 0)
        {   // Pos hit 0, stop sound output
            is_enabled = false;
        }
    }
}

void AudioWave::updateSample()
{
    uint8_t bytePos = nibble_pos / 2;
    uint8_t waveByte = wave_pattern_RAM[bytePos];

    bool useUpperNibble = (nibble_pos % 2) == 0;

    // Get correct nibble
    if (useUpperNibble)
    {
        waveByte = waveByte >> 4;
    }
    else
    {   // Use lower nibble
        waveByte &= 0x0F;
    }

    // Set volume
    if (volume)
    {   // Lower volume via bit-shifting
        waveByte = waveByte >> (volume - 1);
    }
    else
    {   // Output is muted
        waveByte = 0;
    }

    // Update curr_sample
    curr_sample = waveByte;
}

bool AudioWave::isRunning()
{
    logger->trace("sound_length_data: 0x{0:x}, is_enabled: {1:b}, channel_is_enabled: {2:b}",
        sound_length_data,
        is_enabled,
        channel_is_enabled);

    //return (sound_length_data & 0xFF) > 0
    return sound_length_data > 0
        && is_enabled
        && channel_is_enabled;
}