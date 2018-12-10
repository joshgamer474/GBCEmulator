#include <AudioWave.h>
#include <CPU.h>

AudioWave::AudioWave(const uint16_t & register_offset)
    : reg_offset(register_offset)
{
    output_level    = 0;
    frequency_16    = 0;
    frequency       = 0;
    timer           = 0;
    period          = 0;
    sound_length    = 0.0f;
    restart_sound   = false;
    is_enabled      = false;
    stop_output_when_sound_length_ends = false;
}

AudioWave::~AudioWave()
{

}

void AudioWave::setByte(const uint16_t & addr, const uint8_t & val)
{
    uint16_t useAddr = addr - reg_offset;

    if (useAddr < registers.size())
    {
        registers[useAddr] = val;
        parseRegister(useAddr, val);
    }
    else if (addr >= 0xFF30 && addr <= 0xFF3F)
    {   // Wave Pattern RAM
        wave_pattern_RAM[addr - 0xFF30] = val;
    }
    else
    {
        // Log loudly later
    }
}

uint8_t AudioWave::readByte(const uint16_t & addr)
{
    uint16_t useAddr = addr - reg_offset;

    if (useAddr < registers.size())
    {
        return registers[useAddr];
    }
    else if (addr >= 0xFF30 && addr <= 0xFF3F)
    {
        return wave_pattern_RAM[addr - 0xFF30];
    }
    else
    {
        return 0xFF;
    }
}


void AudioWave::parseRegister(const uint8_t & reg, const uint8_t & val)
{
    switch (reg)
    {
    case 0: is_enabled = val & 0x80; break;
    case 1:
        sound_length = val;
        sound_length_seconds = (256.0f - val) * (1.0f / 256.0f);
        break;
    case 2: output_level = (val & 0x60) >> 5; break;
    case 3:
        frequency_16 &= 0xF0;
        frequency_16 |= val;
        break;
    case 4:
        frequency_16 &= 0x0F;
        frequency_16 |= (static_cast<uint16_t>(val) & 0x03) << 8;
        stop_output_when_sound_length_ends = val & 0x40;
        restart_sound = val & 0x80;

        if (restart_sound)
        {
            is_enabled = true;
        }

        // Calculate frequency
        frequency = 131072 / (2048 - frequency_16);

        // Calculate period
        period = CLOCK_SPEED / frequency;
        break;
    }
}

std::vector<uint8_t> AudioWave::run()
{
    timer++;

    if (timer > period)
    {
        generateOutputClock();
        timer -= period;
    }

    return curr_sample;
}

void AudioWave::generateOutputClock()
{
    size_t counter = 0;
    uint32_t use_sound_length;
    uint8_t use_nibble = 0;

    use_sound_length = getSoundLength();

    curr_sample.clear();
    curr_sample.resize(wave_pattern_RAM.size() * 2 * use_sound_length);
    curr_sample.shrink_to_fit();

    for (int i = 0; i < wave_pattern_RAM.size() * 2; i++)
    {   // Get nibble (4 bits)
        use_nibble = wave_pattern_RAM[i / 2];

        if (i % 2 == 0)
        {   // Get upper nibble
            use_nibble = use_nibble >> 4;
        }
        else
        {   // Get lower nibble
            use_nibble &= 0x0F;
        }

        // Check if need to shift nibble
        if (output_level == 0)
        {   // Mute
            return;
        }
        else
        {   // Shift Wave Pattern RAM data
            use_nibble = use_nibble >> (output_level - 1);
        }


        while (use_sound_length > 0)
        {
            curr_sample[counter] = (use_nibble >> (7 - i)) & 0x01;
            use_sound_length--;
            counter++;
        }

        use_sound_length = getSoundLength();
    }

    if (stop_output_when_sound_length_ends)
    {
        is_enabled = false; // Double check this
    }
}

uint32_t AudioWave::getSoundLength()
{
    if (stop_output_when_sound_length_ends)
    {   // Use Sound length data as sound_length
        return sound_length;
    }
    else
    {   // Use frequency as sound_length
        return frequency;
    }
}