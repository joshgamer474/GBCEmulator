#include <boost/test/unit_test.hpp>
#include "../../individual_rom_test.h"

BOOST_AUTO_TEST_CASE(cpu_instrs_06_ld_r_r)
{
    InvividualRomTest test;
    std::string romPath = "../../../blarggtests/cpu_instrs/individual";
    std::string romName = "06-ld r,r.gb";
    test(romPath, romName);
}