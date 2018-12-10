#include "AudioSquare.h"
#include "CPU.h"
#include "Joypad.h"
#include <vector>

AudioSquare::AudioSquare(const uint16_t & register_offset)
    : reg_offset(register_offset)
{
    duty_pos                = 0;
    sweep_shift_num             = 0;
    wave_pattern_duty           = 0;
    sound_length_data           = 0;
    initial_volume_of_envelope  = 0;
    envelope_sweep_num          = 0;
    frequency_16                = 0;
    frequency                   = 0;
    timer                       = 0;
    sweep_time                  = 0.0f;
    sound_length                = 0.0f;
    sweep_decrease              = false;
    envelope_increase           = false;
    stop_output_when_sound_length_ends = false;
    restart_sound               = false;
    is_enabled                  = false;
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
        wave_pattern_duty = (val & 0xC0) >> 6;
        sound_length_data = val & 0x3F;
        sound_length = (64.0f - sound_length_data) * (1.0f / 256.0f);
        break;

    case 2:
        initial_volume_of_envelope = val >> 4;
        envelope_increase = val & BIT3;
        envelope_sweep_num = val & 0x03;
        break;

    case 3:
        frequency_16 &= 0xF0;
        frequency_16 |= val;
        break;

    case 4:
        frequency_16 &= 0x0F;
        frequency_16 |= (static_cast<uint16_t>(val) & 0x03) << 8;
        stop_output_when_sound_length_ends = val & BIT6;
        restart_sound = val & BIT7;

        if (restart_sound)
        {
            is_enabled = true;
        }

        // Calculate frequency
        frequency = 131072 / (2048 - frequency_16);
        //frequency = 1000;

        // Calculate period
        //period = CLOCK_SPEED / frequency;
        period = (2048 - frequency_16) * 4;
        break;
    }
}

uint8_t AudioSquare::getWaveDuty()
{
    switch (wave_pattern_duty)
    {
    case 0: return 0x7F;    // _--- ----
    case 1: return 0x3F;    // __-- ----
    case 2: return 0x0F;    // ____ ----
    case 3: return 0x03;    // ____ __--
    }
}

void AudioSquare::tick()
{
    timer++;

    if (timer >= period)
    {
        generateOutputClock();
        timer -= period;
        duty_pos++;
        duty_pos &= 0x07;
    }
}

void AudioSquare::tickLengthCounter()
{
    if (stop_output_when_sound_length_ends && sound_length_data != 0)
    {
        sound_length_data--;

        // Update register
        registers[1] &= 0xC0;
        registers[1] |= sound_length_data;

        if (sound_length_data == 0)
        {   // Length counter hit 0, stop sound output
            is_enabled = false;
        }
    }
}

void AudioSquare::generateOutputClock()
{
    //size_t counter = 0;
    //uint32_t use_sound_length;
    uint8_t wave_duty = getWaveDuty();
    //uint8_t wave_duty = 0x0F;

    //if (stop_output_when_sound_length_ends)
    //{   // Use Sound length data as counter
    //    use_sound_length = sound_length_data;
    //}
    //else
    //{   // Use frequency as counter
    //    use_sound_length = frequency;
    //}

    if (is_enabled)
    {
        curr_sample = 0;
        return;
    }

    curr_sample = (wave_duty >> (7 - duty_pos)) & 0x01;
}