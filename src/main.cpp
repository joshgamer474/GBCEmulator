#define SDL_MAIN_HANDLED

#include <GBCEmulator.h>
#include <SDLWindow.h>
#include <JoypadXInput.h>
#include <memory>
#include <thread>
#include <SDL.h>

int main(int argc, char **argv)
{
    SDLWindow window;

    if (argc > 1)
    {
        std::string romName = argv[1];
        if (SDLWindow::romIsValid(romName))
        {
            std::shared_ptr<GBCEmulator> emu = std::make_shared<GBCEmulator>(romName, romName + ".log");
            window.hookToEmulator(emu);
        }
    }

    window.run();

    return 0;
}