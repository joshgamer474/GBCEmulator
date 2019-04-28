#include <Fixtures/ROMTestFixture.h>
#include <gtest/gtest.h>
#include <UnitTests.h>

TEST_F(ROMTestFixture, interrupt_time_group)
{
    SetUp(blargg::interrupt_time::group);
}
