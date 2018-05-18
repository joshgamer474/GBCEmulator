#include <boost/test/unit_test.hpp>
#include "../../individual_rom_test.h"

BOOST_AUTO_TEST_CASE(cpu_instrs_05_op_rp)
{
    InvividualRomTest test;
    std::string romPath = "../../../blarggtests/cpu_instrs/individual";
    std::string romName = "05-op rp.gb";
    test(romPath, romName);
}