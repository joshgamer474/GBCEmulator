#include <Fixtures/ROMTestFixture.h>
#include <gtest/gtest.h>
#include <UnitTests.h>

TEST_F(ROMTestFixture, mem_timing_2_01_read_timing)
{
    SetUp(blargg::mem_timing_2::_01_read_timing);
}

TEST_F(ROMTestFixture, mem_timing_2_02_write_timing)
{
    SetUp(blargg::mem_timing_2::_02_write_timing);
}

TEST_F(ROMTestFixture, mem_timing_2_03_modify_timing)
{
    SetUp(blargg::mem_timing_2::_03_modify_timing);
}

TEST_F(ROMTestFixture, mem_timing_2_group)
{
    SetUp(blargg::mem_timing_2::group);
}