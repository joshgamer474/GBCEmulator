#include <Fixtures/ROMTestFixture.h>
#include <gtest/gtest.h>
#include <UnitTests.h>

TEST_F(ROMTestFixture, oam_bug__01_lcd_sync)
{
    SetUp(blargg::oam_bug::_01_lcd_sync);
}

TEST_F(ROMTestFixture, oam_bug_02_causes)
{
    SetUp(blargg::oam_bug::_02_causes);
}

TEST_F(ROMTestFixture, oam_bug_03_non_causes)
{
    SetUp(blargg::oam_bug::_03_non_causes);
}

TEST_F(ROMTestFixture, oam_bug_04_scanline_timing)
{
    SetUp(blargg::oam_bug::_04_scanline_timing);
}

TEST_F(ROMTestFixture, oam_bug_05_timing_bug)
{
    SetUp(blargg::oam_bug::_05_timing_bug);
}

TEST_F(ROMTestFixture, oam_bug_06_timing_no_bug)
{
    SetUp(blargg::oam_bug::_06_timing_no_bug);
}

TEST_F(ROMTestFixture, oam_bug_07_timing_effect)
{
    SetUp(blargg::oam_bug::_07_timing_effect);
}

TEST_F(ROMTestFixture, oam_bug_08_instr_effect)
{
    SetUp(blargg::oam_bug::_08_instr_effect);
}

TEST_F(ROMTestFixture, oam_bug_group)
{
    SetUp(blargg::oam_bug::group);
}
