#include "GBCEmulator.h"

#undef main
int main(int argc, char **argv)
{
    std::string romName;
    if (argc > 1)
    {
        romName = argv[1];
    }
    else
    {
        return -1;
    }

    GBCEmulator emu(romName);
    emu.run();

    return 0;
}