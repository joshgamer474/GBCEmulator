#include <AudioWave.h>
#include <CPU.h>
#include <Joypad.h>

AudioWave::AudioWave(const uint16_t & register_offset, std::shared_ptr<spdlog::logger> _logger, const bool is_color_gb)
    :   reg_offset(register_offset)
    ,   logger(_logger)
    ,   is_color_gb(is_color_gb)
{
    volume          = 0;
    output_volume   = 0;
    frequency_16    = 0;
    frequency       = 0;
    access_ticks    = 0;
    timer           = 64;
    period          = 0;
    nibble_pos      = 0;
    byte_pos        = 0;
    curr_sample     = 0;
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
    access_ticks        = rhs.access_ticks;
    timer               = rhs.timer;
    period              = rhs.period;
    nibble_pos          = rhs.nibble_pos;
    byte_pos            = rhs.byte_pos;
    curr_sample         = rhs.curr_sample;
    is_color_gb         = rhs.is_color_gb;
    sound_length_data   = rhs.sound_length_data;
    restart_sound       = rhs.restart_sound;
    is_enabled          = rhs.is_enabled;
    channel_is_enabled  = rhs.channel_is_enabled;
    stop_output_when_sound_length_ends = rhs.stop_output_when_sound_length_ends;

    return *this;
}

void AudioWave::powerOn()
{
    nibble_pos = 0;
    byte_pos = 0;
    access_ticks = 0;
    curr_sample = 0;
    output_volume = 0;
    is_enabled = false;
}

void AudioWave::setByte(const uint16_t & addr, const uint8_t & val, const bool& extra_length_clock)
{
    bool was_length_enabled = false;
    bool reload_length = false;
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
        sound_length_data = 0x0100 - static_cast<uint16_t>(val);

        /*if (sound_length_data == 0x0100)
        {
            sound_length_data = 0xFF;
            logger->trace("Setting sound_length_data to 0xFF as it was 0x0100, but the register is supposed to be 1 byte");
        }*/

        break;
    case 0xFF1C:    // NR32
        volume = (val & 0x60) >> 5;
        break;
    case 0xFF1D:    // NR33
        frequency_16 &= 0xFF00;
        frequency_16 |= val;
        break;
    case 0xFF1E:    // NR34
        was_length_enabled = stop_output_when_sound_length_ends;
        stop_output_when_sound_length_ends = val & BIT6;
        frequency_16 &= 0x00FF;
        frequency_16 |= (static_cast<uint16_t>(val) & 0x07) << 8;

        // Calculate frequency
        frequency = 131072 / (2048 - frequency_16);

        // Tick length counter at register write while between length ticks
        if (!was_length_enabled && stop_output_when_sound_length_ends && extra_length_clock)
        {
            tickLengthCounter();
        }

        restart_sound = val & BIT7;
        if (restart_sound)
        {
            // Handle DMG triggering on active wave channel
            if (!is_color_gb && is_enabled && channel_is_enabled && timer == 2)
            {
                // Corrupt wave RAM
                uint8_t next_byte = ((nibble_pos + 1) & 0x1F) / 2;
                if (next_byte < 4)
                {
                    wave_pattern_RAM[0] = wave_pattern_RAM[next_byte];
                }
                else
                {
                    next_byte = next_byte & 0x0C;
                    const std::array<uint8_t, 16> wave_RAM = wave_pattern_RAM;
                    for (uint8_t i = 0; i < 4; i++)
                    {
                        wave_pattern_RAM[i] = wave_RAM[next_byte + i];
                    }
                }
            }

            // Check if length counter should be reloaded after the above^ length counter tick
            reload_length = sound_length_data == 0;

            reset();

            if (reload_length && stop_output_when_sound_length_ends && extra_length_clock)
            {
                tickLengthCounter();
            }
        }

        break;
    default:

        if (addr >= 0xFF30 && addr <= 0xFF3F)
        {
            const bool inactive = !(is_enabled && channel_is_enabled);
            if (inactive)
            {
                // Write to wave RAM to the address given, i.e. normally
                wave_pattern_RAM[addr - 0xFF30] = val;
                break;
            }

            // Check if DMG still has access to wave RAM
            if (!is_color_gb && access_ticks == 0)
            {
                // DMG does not have access to wave RAM
                break;
            }

            // Write to wave RAM at the current active/used address
            wave_pattern_RAM[byte_pos] = val;
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
            const bool inactive = !(is_enabled && channel_is_enabled);
            if (inactive)
            {
                return wave_pattern_RAM[addr - 0xFF30];
            }

            // Check if DMG still has access to wave RAM
            if (!is_color_gb && access_ticks == 0)
            {
                return 0xFF;
            }

            return wave_pattern_RAM[byte_pos];
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

    // Add 6 ticks to timer
    timer += 6;

    // Reset nibble pos
    nibble_pos = 0;
    byte_pos = 0;
    access_ticks = 0;

    if (sound_length_data == 0)
    {
        sound_length_data = 0x0100;
        //sound_length_data = 0xFF;
    }
}

void AudioWave::tick()
{
    if (access_ticks > 0)
    {
        access_ticks--;
    }

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
    byte_pos = nibble_pos / 2;
    uint8_t waveByte = wave_pattern_RAM[byte_pos];

    bool useUpperNibble = (nibble_pos % 2) == 0;

    // Reset RAM access ticks
    access_ticks = 2;

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