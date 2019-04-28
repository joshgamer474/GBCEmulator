#include <Fixtures/ROMTestFixture.h>
#include <gtest/gtest.h>
#include <UnitTests.h>

TEST_F(ROMTestFixture, dmg_sounds_01_registers)
{
    SetUp(blargg::dmg_sound::_01_registers);
}

TEST_F(ROMTestFixture, dmg_sounds_02_len_ctr)
{
    SetUp(blargg::dmg_sound::_02_len_ctr);
}

TEST_F(ROMTestFixture, dmg_sounds_03_trigger)
{
    SetUp(blargg::dmg_sound::_03_trigger);
}

TEST_F(ROMTestFixture, dmg_sounds_04_sweep)
{
    SetUp(blargg::dmg_sound::_04_sweep);
}

TEST_F(ROMTestFixture, dmg_sounds_05_sweep_details)
{
    SetUp(blargg::dmg_sound::_05_sweep_details);
}

TEST_F(ROMTestFixture, dmg_sounds_06_overflow_on_trigger)
{
    SetUp(blargg::dmg_sound::_06_overflow_on_trigger);
}

TEST_F(ROMTestFixture, dmg_sounds_07_len_sweep_period_sync)
{
    SetUp(blargg::dmg_sound::_07_len_sweep_period_sync);
}

TEST_F(ROMTestFixture, dmg_sounds_08_len_ctr_during_power)
{
    SetUp(blargg::dmg_sound::_08_len_ctr_during_power);
}

TEST_F(ROMTestFixture, dmg_sounds_09_wave_read_while_on)
{
    SetUp(blargg::dmg_sound::_09_wave_read_while_on);
}

TEST_F(ROMTestFixture, dmg_sounds_10_wave_trigger_while_on)
{
    SetUp(blargg::dmg_sound::_10_wave_trigger_while_on);
}

TEST_F(ROMTestFixture, dmg_sounds_11_regs_after_power)
{
    SetUp(blargg::dmg_sound::_11_regs_after_power);
}

TEST_F(ROMTestFixture, dmg_sounds_12_wave_write_while_on)
{
    SetUp(blargg::dmg_sound::_12_wave_write_while_on);
}

TEST_F(ROMTestFixture, dmg_sounds_group)
{
    SetUp(blargg::dmg_sound::group);
}
