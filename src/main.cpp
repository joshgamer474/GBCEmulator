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
    bool start_emu = false;

    if (argc > 1)
    {
        std::string romName = argv[1];
        if (SDLWindow::romIsValid(romName))
        {
            std::shared_ptr<GBCEmulator> emu = std::make_shared<GBCEmulator>(romName, romName + ".log");
            window.hookToEmulator(emu);
            start_emu = true;
        }
        else
        {
            return -2;
        }
    }

    window.run(start_emu);

    return 0;
}