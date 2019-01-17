#include <AudioNoise.h>
#include "Joypad.h"

AudioNoise::AudioNoise(const uint16_t & register_offset)
    : reg_offset(register_offset)
{
    timer                               = 0;
    initial_volume_of_envelope          = 0;
    envelope_period                     = 0;
    envelope_period_load                = 0;
    shift_clock_frequency               = 0;
    stop_output_when_sound_length_ends  = 0;
    sound_length_data                   = 0;
    dividing_ratio_of_frequencies       = 0;
    volume                              = 0;
    output_volume                       = 0;
    lfsr                                = 0;
    restart_sound                       = false;
    envelope_increase                   = false;
    stop_output_when_sound_length_ends  = false;
    is_enabled                          = false;
    half_counter_step                   = false;
    envelope_running                    = false;
    envelope_enabled                    = false;

    divisors = std::array<uint8_t, 8>({
        8,
        16,
        32,
        48,
        64,
        80,
        96,
        112
    });
}

AudioNoise::~AudioNoise()
{

}

void AudioNoise::setByte(const uint16_t & addr, const uint8_t & val)
{
    uint16_t useAddr = addr - reg_offset;

    if (useAddr <= 4)
    {
        parseRegister(useAddr, val);
    }
    else
    {

    }
}

uint8_t AudioNoise::readByte(const uint16_t & addr)
{
    uint16_t useAddr = addr - reg_offset;
    uint8_t ret = 0;
  
    switch (useAddr)
    {
    case 0:
        ret = sound_length_data & 0x3F;
        ret |= 0xC0;    // Unused bits are 1s
        break;
    case 1:
        ret = initial_volume_of_envelope << 4;
        ret |= (envelope_increase << 3);
        ret |= (envelope_period_load & 0x07);
        break;
    case 2:
        ret = (shift_clock_frequency & 0x0F) << 4;
        ret |= (static_cast<uint8_t>(half_counter_step) << 3);
        ret |= (dividing_ratio_of_frequencies & 0x07);
        break;
    case 3:
        ret = (static_cast<uint8_t>(stop_output_when_sound_length_ends) << 6);
        ret |= 0xBF;    // Unused bits are 1s
        break;
    default:
        ret = 0xFF;
    }

    return ret;
}


void AudioNoise::parseRegister(const uint8_t & reg, const uint8_t & val)
{
    switch (reg)
    {
    case 0:
        sound_length_data = val & 0x3F;
        break;

    case 1:
        initial_volume_of_envelope  = val >> 4;
        envelope_increase           = val & BIT3;
        envelope_period_load        = val & 0x07;

        if (val & 0xF8)
        {
            envelope_enabled = true;
        }
        else
        {
            envelope_enabled = false;
        }

        break;

    case 2:
        shift_clock_frequency           = val >> 4;
        half_counter_step               = val & BIT3;
        dividing_ratio_of_frequencies   = val & 0x07;
        break;

    case 3:
        restart_sound                       = val & BIT7;
        stop_output_when_sound_length_ends  = val & BIT6;

        if (restart_sound)
        {   // Sound turning on
            reset();
        }
        break;
    }
}

void AudioNoise::reset()
{
    is_enabled = true;
    envelope_running = true;

    // Reload Timer
    timer = divisors[dividing_ratio_of_frequencies] << shift_clock_frequency;
    
    // Reload Length timer
    if (sound_length_data == 0)
    {
        sound_length_data = 0x40;
    }

    // Reload volume envelope
    reloadPeriod(envelope_period, envelope_period_load);
    volume = initial_volume_of_envelope;

    // Reset LFSR register
    lfsr = 0x7FFF;
}

void AudioNoise::tick()
{
    if (timer == 0)
    {
        return;
    }

    if (timer > 0)
    {
        timer--;
    }

    if (timer == 0)
    {   // Reload timer
        timer = divisors[dividing_ratio_of_frequencies] << shift_clock_frequency;

        uint8_t xor_result = (lfsr & 0x01) ^ ((lfsr >> 1) & 0x01);
        lfsr = lfsr >> 1;
        lfsr |= (xor_result << 14);

        if (half_counter_step)
        {
            lfsr &= 0xFFBF;             // Mask off bit 6
            lfsr |= (xor_result << 6);  // Put XOR result into bit 6
        }

        if (is_enabled &&
            envelope_enabled &&
            (lfsr & 0x01) == 0)
        {
            output_volume = volume;
        }
        else
        {
            output_volume = 0;
        }
    }
}

void AudioNoise::tickLengthCounter()
{
    if (stop_output_when_sound_length_ends)
    {
        if (sound_length_data == 0)
        {
            return;
        }
        else
        {   // sound_length_data > 0;
            sound_length_data--;
        }

        if (sound_length_data == 0)
        {   // Length counter hit 0, stop sound output
            is_enabled = false;
        }
    }
}

void AudioNoise::tickVolumeEnvelope()
{
    if (envelope_period == 0)
    {
        return;
    }
    else
    {   // envelope_period > 0
        envelope_period--;
    }

    if (envelope_period == 0)
    {
        reloadPeriod(envelope_period, envelope_period_load);

        if (envelope_running && envelope_period_load)
        {
            if (envelope_increase && volume < 0x0F)
            {
                volume++;
            }
            else if (!envelope_increase && volume > 0)
            {
                volume--;
            }
        }

        if (volume == 0 ||
            volume == 0x0F)
        {
            envelope_running = false;
        }
    }
}

void AudioNoise::reloadPeriod(uint8_t & period, const uint8_t & periodLoad)
{
    if (periodLoad > 0)
    {
        period = periodLoad;
    }
    else
    {
        period = 8;
    }
}