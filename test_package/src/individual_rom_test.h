#ifndef INDIVIDUAL_ROM_HEADER_H
#define INDIVIDUAL_ROM_HEADER_H
#include <string>
#include <boost/filesystem.hpp>

struct InvividualRomTest
{
public:
    InvividualRomTest();
    ~InvividualRomTest();
    void operator()(std::string romPath, std::string romName);
    void deleteLogFile(boost::filesystem::path logPath);
private:
    boost::filesystem::path logPath;
};
#endif