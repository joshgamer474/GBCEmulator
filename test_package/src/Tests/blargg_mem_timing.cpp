#include <Fixtures/ROMTestFixture.h>
#include <gtest/gtest.h>
#include <UnitTests.h>

TEST_F(ROMTestFixture, mem_timing_01_read_timing)
{
    SetUp(blargg::mem_timing::_01_read_timing);
}

TEST_F(ROMTestFixture, mem_timing_02_write_timing)
{
    SetUp(blargg::mem_timing::_02_write_timing);
}

TEST_F(ROMTestFixture, mem_timing_03_modify_timing)
{
    SetUp(blargg::mem_timing::_03_modify_timing);
}

TEST_F(ROMTestFixture, mem_timing_group)
{
    SetUp(blargg::mem_timing::group);
}