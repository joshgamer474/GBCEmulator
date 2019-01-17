#define SDL_MAIN_HANDLED
#include "GBCEmulator.h"
#include <thread>
#include <SDL.h>

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

    GBCEmulator emu(romName, romName + ".log");

    // Have emulator tick in its own thread
    std::thread thread([&]()
    {
        emu.run();
    });

    // Process input here
    SDL_Event event;
    bool run = true;
    while (run)
    {
        SDL_PollEvent(&event);

        switch (event.type)
        {
        case SDL_KEYDOWN:

            switch (event.key.keysym.sym)
            {
            case SDLK_ESCAPE:
                emu.stop();
                run = false;
                break;
            }
        }
    }

    // Close emulator thread
    thread.join();

    return 0;
}