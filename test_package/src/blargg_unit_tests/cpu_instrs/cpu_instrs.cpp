#include <boost/test/unit_test.hpp>
#include "../../group_rom_test.h"

BOOST_AUTO_TEST_CASE(cpu_instrs)
{
    GroupRomTest test;
    std::string romPath = "../../../blarggtests/cpu_instrs";
    std::string romName = "cpu_instrs.gb";
    test(romPath, romName, 11);
}