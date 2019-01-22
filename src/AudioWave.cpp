#include <AudioWave.h>
#include <CPU.h>
#include <Joypad.h>

AudioWave::AudioWave(const uint16_t & register_offset)
    : reg_offset(register_offset)
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

void AudioWave::setByte(const uint16_t & addr, const uint8_t & val)
{
    switch (addr)
    {
    case 0xFF1A:    // NR30
        channel_is_enabled = val & BIT7;
        break;
    case 0xFF1B:    // NR31
        sound_length_load = 0xFF - val;
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

uint8_t AudioWave::readByte(const uint16_t & addr)
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
    return sound_length_data > 0;
}