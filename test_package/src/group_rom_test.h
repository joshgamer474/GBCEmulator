#ifndef GROUP_ROM_HEADER_H
#define GROUP_ROM_HEADER_H
#include <string>
#include <boost/filesystem.hpp>

struct GroupRomTest
{
public:
    GroupRomTest();
    ~GroupRomTest();
    void operator()(std::string romPath, std::string romName, int numOfIndividualTests);
    void deleteLogFile(boost::filesystem::path logPath);
    bool allTestsPassed(std::string lastLogLine, int numOfIndividualTests);
private:
    boost::filesystem::path logPath;
    bool passed;
};
#endif