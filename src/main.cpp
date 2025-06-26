#define SDL_MAIN_HANDLED

#include <GBCEmulator.h>

#include <SDLWindow.h>
#include <JoypadXInput.h>
#include <Util.h>

#include <memory>
#include <string>
#include <thread>

#include <SDL3/SDL.h>

int main(int argc, char **argv)
{
    SDLWindow window;
    bool start_emu = false;

    if (argc > 1)
    {
        std::string romName = argv[1];
        if (SDLWindow::romIsValid(romName))
        {
            // Generate log path from ROM name and OS
            const std::string logFile = generate_log_path(romName);
            std::shared_ptr<GBCEmulator> emu = std::make_shared<GBCEmulator>(romName, logFile);

            window.hookToEmulator(emu);
            start_emu = true;
        }
    }

    window.run(start_emu);

    return 0;
}