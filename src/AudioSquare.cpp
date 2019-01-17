#include "AudioSquare.h"
#include "CPU.h"
#include "Joypad.h"
#include <vector>

AudioSquare::AudioSquare(const uint16_t & register_offset)
    : reg_offset(register_offset)
{
    duty_pos                    = 0;
    sweep_period                = 0;
    sweep_period_load           = 0;
    sweep_shift                 = 0;
    wave_pattern_duty           = 0;
    sound_length_data           = 0;
    initial_volume_of_envelope  = 0;
    envelope_period_load        = 0;
    envelope_period             = 0;
    frequency_16                = 0;
    sweep_frequency_16          = 0;
    timer                       = 64;
    curr_sample                 = 0;
    volume                      = 0;
    output_volume               = 0;
    sweep_decrease              = false;
    sweep_running               = false;
    envelope_increase           = false;
    envelope_running            = false;
    envelope_enabled            = false;
    stop_output_when_sound_length_ends = false;
    restart_sound               = false;
    is_enabled                  = false;

    initWaveDutyTable();
}

AudioSquare::~AudioSquare()
{

}

void AudioSquare::setByte(const uint16_t & addr, const uint8_t & val)
{
    uint16_t useAddr = addr - reg_offset;

    if (useAddr <= 4)
    {
        parseRegister(useAddr, val);
    }
    else
    {
        // log error ?
    }
}

uint8_t AudioSquare::readByte(const uint16_t & addr)
{
    uint16_t useAddr = addr - reg_offset;
    uint8_t ret = 0;

    if (useAddr <= 4)
    {
        switch (useAddr)
        {
        case 0:
            ret = sweep_period_load << 4;
            ret |= (static_cast<uint8_t>(sweep_decrease) << 3);
            ret |= (sweep_shift & 0x07);
            break;
        case 1:
            ret = (wave_pattern_duty << 6) & 0xC0; // Only bits 6-7 are readable
            ret |= 0x3F;    // Unused bits are 1s
            break;
        case 2:
            ret = initial_volume_of_envelope << 4;
            ret |= (static_cast<uint8_t>(envelope_increase) << 3);
            ret |= (envelope_period_load & 0x07);
            break;
        case 3:
            ret = 0xFF; // Not readable
            break;
        case 4:
            // Only bit 6 is readable
            ret = (static_cast<uint8_t>(stop_output_when_sound_length_ends) << 6);
            ret |= 0xBF;    // Unused bits are 1s
            break;
        }

        return ret;
    }
    else
    {
        return 0xFF;
    }
}


void AudioSquare::parseRegister(const uint8_t & reg, const uint8_t & val)
{
    switch (reg)
    {
    case 0:
        sweep_period_load   = (val >> 4) & 0x07;
        sweep_decrease      = val & BIT3;
        sweep_shift         = val & 0x07;
        break;

    case 1:
        wave_pattern_duty = (val >> 6) & 0x03;
        sound_length_data = val & 0x3F;
        break;

    case 2:
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

    case 3:
        frequency_16 &= 0xFF00;
        frequency_16 |= val;
        break;

    case 4:
        frequency_16 &= 0x00FF;
        frequency_16 |= (static_cast<uint16_t>(val) & 0x07) << 8;
        stop_output_when_sound_length_ends = val & BIT6;

        restart_sound = val & BIT7;
        if (restart_sound)
        {
            reset();
        }
        break;
    }
}

void AudioSquare::reset()
{
    is_enabled = true;
    envelope_running = true;

    // Calculate period
    period = (2048 - frequency_16) * 4;

    // Reload frequency period
    timer = (timer & 0x03) | period;    // Lower 2 bits of frequency timer are not modified

    // Reload Envelope period
    reloadPeriod(envelope_period, envelope_period_load);

    // Reload Envelope volume
    volume = initial_volume_of_envelope;

    if (sound_length_data == 0)
    {
        sound_length_data = 0x40;
    }

    /// Reset Sweep
    sweep_frequency_16 = frequency_16;

    // Reload Sweep period
    reloadPeriod(sweep_period, sweep_period_load);

    if (sweep_period == 0)
    {   // Some behavior on gbdev.gg8.se/wiki
        sweep_period = 8;
    }

    // Check if Sweep is enabled
    if (sweep_period != 0 || sweep_shift != 0)
    {
        sweep_running = true;
    }
    else
    {
        sweep_running = false;
    }

    if (sweep_shift != 0)
    {   // Calculate frequency
        calculateSweepFrequency();
    }
}

void AudioSquare::initWaveDutyTable()
{
    wave_duty_table[0] = { 0, 1, 1, 1, 1, 1, 1, 1 };    // 12.5% duty
    wave_duty_table[1] = { 0, 0, 1, 1, 1, 1, 1, 1 };    // 25% duty
    wave_duty_table[2] = { 0, 0, 0, 0, 1, 1, 1, 1 };    // 50% duty
    wave_duty_table[3] = { 0, 0, 0, 0, 0, 0, 1, 1 };    // 75% duty
}

void AudioSquare::tick()
{
    if (timer > 0)
    {
        timer--;
    }

    if (timer == 0)
    {
        // Calculate period
        period = (2048 - frequency_16) * 4;

        timer = period; // Reload frequency period
        duty_pos++;
        duty_pos &= 0x07;
        
        curr_sample = wave_duty_table[wave_pattern_duty][duty_pos];
    }

    // Update output
    if (is_enabled &&
        envelope_enabled &&
        curr_sample > 0)
    {
        output_volume = volume;
    }
    else
    {
        output_volume = 0;
    }
}

void AudioSquare::tickLengthCounter()
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

// http://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware
void AudioSquare::tickVolumeEnvelope()
{
    if (envelope_period == 0)
    {
        return;
    }

    envelope_period--;

    if (envelope_period == 0)
    {
        reloadPeriod(envelope_period, envelope_period_load);

        if (envelope_running && envelope_period_load)
        {   // Envelope is enabled
            // Increase or decrease volume
            if (envelope_increase && volume < 0x0F)
            {
                volume++;
            }
            else if (!envelope_increase && volume > 0)
            {
                volume--;
            }
        }

        // Check if Envelope should be disabled
        if (volume == 0x00 ||
            volume == 0x0F)
        {
            envelope_running = false;
        }
    }
}
// http://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware
// Frequency Sweep
void AudioSquare::tickSweep()
{
    if (sweep_period == 0)
    {
        return;
    }

    sweep_period--;

    if (sweep_period == 0)
    {   // Reload period
        if (sweep_period_load == 0)
        {   // Some behavior from gbdev.gg8.se/wiki
            sweep_period = 8;
        }
        else
        {
            reloadPeriod(sweep_period, sweep_period_load);
        }

        sweep_frequency_16 = frequency_16;

        if (sweep_period != 0 || sweep_shift != 0)
        {
            sweep_running = true;
        }
        else
        {
            sweep_running = false;
        }

        if (sweep_running && sweep_period_load > 0)
        {
            uint16_t newFrequency = calculateSweepFrequency();

            if (newFrequency <= 2047 && sweep_shift > 0)
            {   // Set variables to use new frequency
                sweep_frequency_16 = newFrequency;
                frequency_16 = newFrequency;
                calculateSweepFrequency();  
            }
        }
    }

}

uint16_t AudioSquare::calculateSweepFrequency()
{
    uint16_t freq = sweep_frequency_16 >> sweep_shift;

    if (sweep_decrease)
    {
        freq = sweep_frequency_16 - freq;
    }
    else
    {
        freq = sweep_frequency_16 + freq;
    }

    // Overflow check
    if (freq > 2047)
    {   // Calculated frequency cannot be above 2047, turn off channel
        is_enabled = false;
        sweep_running = false;
    }

    return freq;
}

void AudioSquare::reloadPeriod(uint8_t & period, const uint8_t & periodLoad)
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