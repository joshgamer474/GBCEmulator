#include <Fixtures/ROMTestFixture.h>
#include <gtest/gtest.h>
#include <UnitTests.h>

TEST_F(ROMTestFixture, instr_timing_group)
{
    SetUp(blargg::instr_timing::group);
}