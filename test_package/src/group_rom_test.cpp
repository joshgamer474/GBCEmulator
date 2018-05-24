#include <boost/test/unit_test.hpp>
#include "group_rom_test.h"
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/range/algorithm/count.hpp>
#include <GBCEmulator.h>
#include <thread>

GroupRomTest::GroupRomTest()
    : passed(false)
{
}

GroupRomTest::~GroupRomTest()
{
    if (passed)
    {
        deleteLogFile(logPath);
    }
}

void GroupRomTest::operator()(std::string _romPath, std::string _romName, int numOfIndividualTests)
{
    std::string logName = _romName + ".log";
    boost::filesystem::path romPath = boost::filesystem::path(_romPath);
    romPath.make_preferred();
    romPath.append(_romName);

    logPath = boost::filesystem::path(logName);
    BOOST_TEST_MESSAGE("Testing rom at " << romPath.string());

    // Delete old log file if exists
    deleteLogFile(logPath);

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
    bool finished;
    std::string line;
    finished = false;
    while (!finished)
    {
        boost::filesystem::ifstream logFile(logPath);
        while (logFile)
        {
            std::getline(logFile, line);
            if (boost::contains(line, "ok") || boost::contains(line, "Failed"))
            {
                emu.stop();
                finished = true;
                break;
            }
        }

        boost::this_thread::sleep(boost::posix_time::milliseconds(500));
    }

    thread.join();

    // Check to see if all of the individual tests passed
    passed = allTestsPassed(line, numOfIndividualTests);
    BOOST_REQUIRE_MESSAGE(passed == true, "Test failed: " << line);

    BOOST_TEST_MESSAGE("Test passed!");
}

void GroupRomTest::deleteLogFile(boost::filesystem::path logPath)
{
    if (boost::filesystem::exists(logPath))
    {
        boost::filesystem::remove(logPath);
    }
}

bool GroupRomTest::allTestsPassed(std::string lastLogLine, int numOfIndividualTests)
{
    int numOfOccurances = 0; 
    std::size_t pos = pos = lastLogLine.find("ok", 0);;
    while (pos != std::string::npos)
    {
        pos = lastLogLine.find("ok", ++pos);
        numOfOccurances++;
    }

    return numOfIndividualTests == numOfOccurances;
}