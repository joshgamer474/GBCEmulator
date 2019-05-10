#include <Fixtures/ROMTestFixture.h>
#include <gtest/gtest.h>
#include <UnitTests.h>

TEST_F(ROMTestFixture, halt_bug_group)
{
    SetUp(blargg::halt_bug::group);
}