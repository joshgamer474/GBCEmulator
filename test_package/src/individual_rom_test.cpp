#include <boost/test/unit_test.hpp>
#include "individual_rom_test.h"
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <GBCEmulator.h>
#include <string>
#include <thread>

InvividualRomTest::InvividualRomTest()
{
}

void InvividualRomTest::operator()(std::string _romPath, std::string _romName)
{
    std::string logName = _romName + ".log";
    boost::filesystem::path romPath = boost::filesystem::path(_romPath);
    romPath.make_preferred();
    romPath.append(_romName);

    boost::filesystem::path logPath(logName);
    BOOST_TEST_MESSAGE("Testing rom at " << romPath.string());

    // Delete old log file if exists
    if (boost::filesystem::exists(logPath))
    {
        boost::filesystem::remove(logPath);
    }

    // Create emulator obj
    BOOST_TEST_MESSAGE("Creating emulator");
    GBCEmulator emu(romPath.string(), logName);

    // Run emulator with rom loaded
    BOOST_TEST_MESSAGE("Running emulator");
    std::thread thread([&]()
    {
        emu.run();
    });

    // Check log to see if test is finished
    bool finished, passed;
    std::string line;
    finished = passed = false;
    while (!finished)
    {
        boost::filesystem::ifstream logFile(logPath);
        while (logFile)
        {
            std::getline(logFile, line);
            if (boost::contains(line, "Passed"))
            {
                emu.stop();
                finished = true;
                passed = true;
                break;
            }
            else if (boost::contains(line, "Failed"))
            {
                emu.stop();
                finished = true;
                passed = false;
                break;
            }
        }

        boost::this_thread::sleep(boost::posix_time::milliseconds(300));
    }

    thread.join();

    BOOST_ASSERT_MSG(passed == true, std::stringstream() << "Test failed: " << line);

    BOOST_TEST_MESSAGE("Test passed!");
}