#ifndef INDIVIDUAL_ROM_HEADER_H
#define INDIVIDUAL_ROM_HEADER_H
#include <string>

struct InvividualRomTest
{
public:
    InvividualRomTest();
    void operator()(std::string romPath, std::string romName);
};
#endif