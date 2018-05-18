#include <boost/test/unit_test.hpp>
#include "../../individual_rom_test.h"

BOOST_AUTO_TEST_CASE(cpu_instrs_02_interrupts)
{
    InvividualRomTest test;
    std::string romPath = "../../../blarggtests/cpu_instrs/individual";
    std::string romName = "02-interrupts.gb";
    test(romPath, romName);
}