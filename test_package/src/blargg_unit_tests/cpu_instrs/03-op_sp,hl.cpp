#include <boost/test/unit_test.hpp>
#include "../../individual_rom_test.h"

BOOST_AUTO_TEST_CASE(cpu_instrs_03_op_sp_hl)
{
    InvividualRomTest test;
    std::string romPath = "../../../blarggtests/cpu_instrs/individual";
    std::string romName = "03-op sp,hl.gb";
    test(romPath, romName);
}