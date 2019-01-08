#include "AudioSquare.h"
#include "CPU.h"
#include "Joypad.h"
#include <vector>

AudioSquare::AudioSquare(const uint16_t & register_offset)
    : reg_offset(register_offset)
{
    duty_pos                    = 0;
    sweep_shift_num             = 0;
    wave_pattern_duty           = 0;
    sound_length_data           = 0;
    initial_volume_of_envelope  = 0;
    envelope_period_load        = 0;
    envelope_period             = 0;
    frequency_16                = 0;
    frequency                   = 0;
    timer                       = 64;
    curr_sample                 = 0;
    volume                      = 0;
    output_volume               = 0;
    sweep_time                  = 0.0f;
    sound_length                = 0.0f;
    sweep_decrease              = false;
    envelope_increase           = false;
    envelope_running            = false;
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

    if (useAddr < registers.size())
    {
        registers[useAddr] = val;
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

    if (useAddr < registers.size())
    {
        return registers[useAddr];
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
        sweep_time = (val & 0x70) >> 4;
        sweep_decrease = val & BIT3;
        sweep_shift_num = val & 0x07;

        sweep_time *= (1.0f / 128.0f);
        break;

    case 1:
        wave_pattern_duty = (val >> 6) & 0x03;
        sound_length_data = val & 0x3F;
        sound_length = (64.0f - sound_length_data) * (1.0f / 256.0f);
        break;

    case 2:
        initial_volume_of_envelope  = val >> 4;
        envelope_increase           = val & BIT3;
        envelope_period_load        = val & 0x03;

        envelope_period = envelope_period_load;
        volume = initial_volume_of_envelope;
        break;

    case 3:
        frequency_16 &= 0xFF00;
        frequency_16 |= val;

        break;

    case 4:
        frequency_16 &= 0x00FF;
        frequency_16 |= (static_cast<uint16_t>(val) & 0x07) << 8;
        stop_output_when_sound_length_ends = val & BIT6;

        // Calculate frequency
        frequency = 131072 / (2048 - frequency_16);

        if (restart_sound == false && (val & BIT7))
        {
            reset();
        }
        restart_sound = val & BIT7;

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
    timer = period;

    // Reload envelope period
    envelope_period = envelope_period_load;

    // Reload envelope volume
    volume = initial_volume_of_envelope;

    if (sound_length_data == 0)
    {
        sound_length_data = 63;

        // Update register
        registers[1] &= 0xC0;
        registers[1] |= sound_length_data;
    }

    /// TODO
    // Channel volume from NRx2 is reloaded
    // Reset Channel 1's sweep
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
    }

    curr_sample = wave_duty_table[wave_pattern_duty][duty_pos];

    // Update output
    if (is_enabled &&
        initial_volume_of_envelope != 0 &&
        curr_sample != 0)
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
    if (stop_output_when_sound_length_ends && sound_length_data > 0)
    {
        sound_length_data--;

        // Update register
        registers[1] &= 0xC0;
        registers[1] |= sound_length_data;

        if (sound_length_data == 0)
        {   // Length counter hit 0, stop sound output
            is_enabled = false;
            curr_sample = 0;
        }
    }
}

void AudioSquare::tickVolumeEnvelope()
{
    if (envelope_period > 0)
    {
        envelope_period--;

        if (envelope_period == 0)
        {   // Reload period
            if (envelope_period_load > 0)
            {
                envelope_period = envelope_period_load;
            }
            else
            {
                envelope_period = 8;
            }

            if (envelope_running && envelope_period > 0)
            {   // Envelope is enabled and running
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

}