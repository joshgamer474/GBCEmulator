#include <AudioSquare.h>
#include <vector>

AudioSquare::AudioSquare(const uint16_t & register_offset, std::string filename)
    : reg_offset(register_offset)
{
    sweep_shift_num             = 0;
    wave_pattern_duty           = 0;
    sound_length_data           = 0;
    initial_volume_of_envelope  = 0;
    envelope_sweep_num          = 0;
    frequency_16                = 0;
    frequency                   = 0;
    sweep_time                  = 0.0f;
    sound_length                = 0.0f;
    sweep_decrease              = false;
    envelope_increase           = false;
    stop_output_when_sound_length_ends = false;
    restart_sound               = false;

    outFile = std::make_unique<std::ofstream>(filename, std::ios::binary);
}

AudioSquare::~AudioSquare()
{
    outFile->close();
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
        sweep_decrease = val & 0x08;
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
        envelope_increase = val & 0x08;
        envelope_sweep_num = val & 0x03;
        break;

    case 3:
        frequency_16 &= 0xF0;
        frequency_16 |= val;
        break;

    case 4:
        frequency_16 &= 0x0F;
        frequency_16 |= (static_cast<uint16_t>(val) & 0x03) << 8;
        stop_output_when_sound_length_ends = val & 0x40;
        restart_sound = val & 0x80;

        // Calculate frequency
        frequency = 131072 / (2048 - frequency_16);
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

void AudioSquare::run()
{
    size_t counter = 0;
    uint32_t use_sound_length;
    uint8_t wave_duty = getWaveDuty();
    std::vector<uint8_t> audio_out;

    if (stop_output_when_sound_length_ends)
    {   // Use Sound length data as counter
        use_sound_length = sound_length_data;
    }
    else
    {   // Use frequency as counter
        use_sound_length = frequency;
    }

    audio_out.resize(8 * use_sound_length);

    for (int i = 0; i < 8; i++)
    {
        while (use_sound_length > 0)
        {
            audio_out[counter] = (wave_duty >> (7 - i)) & 0x01;
            use_sound_length--;
            counter++;
        }

        if (stop_output_when_sound_length_ends)
        {   // Use Sound length data as counter
            use_sound_length = sound_length_data;
        }
        else
        {   // Use frequency as counter
            use_sound_length = frequency;
        }
    }

    outFile->write(reinterpret_cast<const char *>(audio_out.data()), audio_out.size());
}