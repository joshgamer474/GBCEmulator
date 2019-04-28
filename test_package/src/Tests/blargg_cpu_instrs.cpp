#include <Fixtures/ROMTestFixture.h>
#include <gtest/gtest.h>
#include <UnitTests.h>

TEST_F(ROMTestFixture, cpu_instrs_01_special)
{
    SetUp(blargg::cpu_instrs::_01_special);
}

TEST_F(ROMTestFixture, cpu_instrs_02_interrupts)
{
    SetUp(blargg::cpu_instrs::_02_interrupts);
}

TEST_F(ROMTestFixture, cpu_instrs_03_op_sp_hl)
{
    SetUp(blargg::cpu_instrs::_03_op_sp_hl);
}

TEST_F(ROMTestFixture, cpu_instrs_04_op_r_imm)
{
    SetUp(blargg::cpu_instrs::_04_op_r_imm);
}

TEST_F(ROMTestFixture, cpu_instrs_05_op_rp)
{
    SetUp(blargg::cpu_instrs::_05_op_rp);
}

TEST_F(ROMTestFixture, cpu_instrs_06_ld_r_r)
{
    SetUp(blargg::cpu_instrs::_06_ld_r_r);
}

TEST_F(ROMTestFixture, cpu_instrs_07_jr_jp_call_ret_rst)
{
    SetUp(blargg::cpu_instrs::_07_jr_jp_call_ret_rst);
}

TEST_F(ROMTestFixture, cpu_instrs_08_misc_instrs)
{
    SetUp(blargg::cpu_instrs::_08_misc_instrs);
}

TEST_F(ROMTestFixture, cpu_instrs_09_op_r_r)
{
    SetUp(blargg::cpu_instrs::_09_op_r_r);
}

TEST_F(ROMTestFixture, cpu_instrs_10_bit_ops)
{
    SetUp(blargg::cpu_instrs::_10_bit_ops);
}

TEST_F(ROMTestFixture, cpu_instrs_11_op_a_hl)
{
    SetUp(blargg::cpu_instrs::_11_op_a_hl);
}

TEST_F(ROMTestFixture, cpu_instrs_group)
{
    SetUp(blargg::cpu_instrs::group);
}
