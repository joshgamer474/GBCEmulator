#include <Fixtures/ROMTestFixture.h>
#include <gtest/gtest.h>
#include <UnitTests.h>

TEST_F(ROMTestFixture, cgb_sounds_01_registers)
{
    SetUp(blargg::cgb_sound::_01_registers);
}

TEST_F(ROMTestFixture, cgb_sounds_02_len_ctr)
{
    SetUp(blargg::cgb_sound::_02_len_ctr);
}

TEST_F(ROMTestFixture, cgb_sounds_03_trigger)
{
    SetUp(blargg::cgb_sound::_03_trigger);
}

TEST_F(ROMTestFixture, cgb_sounds_04_sweep)
{
    SetUp(blargg::cgb_sound::_04_sweep);
}

TEST_F(ROMTestFixture, cgb_sounds_05_sweep_details)
{
    SetUp(blargg::cgb_sound::_05_sweep_details);
}

TEST_F(ROMTestFixture, cgb_sounds_06_overflow_on_trigger)
{
    SetUp(blargg::cgb_sound::_06_overflow_on_trigger);
}

TEST_F(ROMTestFixture, cgb_sounds_07_len_sweep_period_sync)
{
    SetUp(blargg::cgb_sound::_07_len_sweep_period_sync);
}

TEST_F(ROMTestFixture, cgb_sounds_08_len_ctr_during_power)
{
    SetUp(blargg::cgb_sound::_08_len_ctr_during_power);
}

TEST_F(ROMTestFixture, cgb_sounds_09_wave_read_while_on)
{
    SetUp(blargg::cgb_sound::_09_wave_read_while_on);
}

TEST_F(ROMTestFixture, cgb_sounds_10_wave_trigger_while_on)
{
    SetUp(blargg::cgb_sound::_10_wave_trigger_while_on);
}

TEST_F(ROMTestFixture, cgb_sounds_11_regs_after_power)
{
    SetUp(blargg::cgb_sound::_11_regs_after_power);
}

TEST_F(ROMTestFixture, cgb_sounds_12_wave)
{
    SetUp(blargg::cgb_sound::_12_wave);
}

TEST_F(ROMTestFixture, cgb_sounds_group)
{
    SetUp(blargg::cgb_sound::group);
}
