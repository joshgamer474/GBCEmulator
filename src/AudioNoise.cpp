#include <AudioNoise.h>

AudioNoise::AudioNoise(const uint16_t & register_offset)
    : reg_offset(register_offset)
{
    initial_volume_of_envelope          = 0;
    envelope_sweep_num                  = 0;
    shift_clock_frequency               = 0;
    counter_step                        = 0;
    sound_length                        = 0.0f;
    dividing_ratio_of_frequencies       = 0.0f;
    frequency                           = 0.0f;
    restart_sound                       = false;
    envelope_increase                   = false;
    stop_output_when_sound_length_ends  = false;
}

AudioNoise::~AudioNoise()
{

}

void AudioNoise::setByte(const uint16_t & addr, const uint8_t & val)
{
    uint16_t useAddr = addr - reg_offset;

    if (useAddr < registers.size())
    {
        registers[useAddr] = val;
        parseRegister(useAddr, val);
    }
    else
    {

    }
}

uint8_t AudioNoise::readByte(const uint16_t & addr)
{
    uint16_t useAddr = addr - reg_offset;

    if (useAddr < registers.size())
    {
        return registers[useAddr];
    }
    else
    {
        return 0xFF;
    }
}


void AudioNoise::parseRegister(const uint8_t & reg, const uint8_t & val)
{
    switch (reg)
    {
    case 0:
        sound_length = (256.0f - val) * (1.0f / 256.0f);
        break;

    case 1:
        initial_volume_of_envelope = val >> 4;
        envelope_increase = val & 0x08;
        envelope_sweep_num = val & 0x03;
        break;

    case 2:
        shift_clock_frequency = val >> 4;
        if (val & 0x08)
        {
            counter_step = 7;
        }
        else
        {
            counter_step = 15;
        }
        dividing_ratio_of_frequencies = val & 0x03;

        // Calculate frequency
        if (dividing_ratio_of_frequencies == 0)
        {
            frequency = 524288.0f / 0.5;
        }
        else
        {
            frequency = 524288.0f / dividing_ratio_of_frequencies;
        }
        frequency /= std::pow(2, shift_clock_frequency + 1);
        break;

    case 3:
        restart_sound = val & 0x80;
        counter_step = val & 0x40;
        break;
    }
}