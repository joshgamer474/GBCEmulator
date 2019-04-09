#define SDL_MAIN_HANDLED
#include <GBCEmulator.h>
#include <JoypadXInput.h>
#include <thread>
#include <SDL.h>

int main(int argc, char **argv)
{
    std::string romName;
    if (argc <= 1)
    {
        return -1;
    }

    romName = argv[1];

    GBCEmulator emu(romName, romName + ".log");

    // Setup XInput Controller
    std::shared_ptr<Joypad> joypad = emu.get_Joypad();
    std::shared_ptr<JoypadXInput> joypadx = std::make_shared<JoypadXInput>(joypad);

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
        {
            switch (event.key.keysym.sym)
            {
            case SDLK_ESCAPE:
                emu.stop();
                run = false;
                break;
            case SDLK_w: joypad->set_joypad_button(Joypad::BUTTON::UP);     break;
            case SDLK_a: joypad->set_joypad_button(Joypad::BUTTON::LEFT);   break;
            case SDLK_s: joypad->set_joypad_button(Joypad::BUTTON::DOWN);   break;
            case SDLK_d: joypad->set_joypad_button(Joypad::BUTTON::RIGHT);  break;
            case SDLK_z: joypad->set_joypad_button(Joypad::BUTTON::A);      break;
            case SDLK_x: joypad->set_joypad_button(Joypad::BUTTON::B);      break;
            case SDLK_m: joypad->set_joypad_button(Joypad::BUTTON::START);  break;
            case SDLK_n: joypad->set_joypad_button(Joypad::BUTTON::SELECT); break;
            }
            break;
        }
        case SDL_KEYUP:
        {
            switch (event.key.keysym.sym)
            {
            case SDLK_w: joypad->release_joypad_button(Joypad::BUTTON::UP);     break;
            case SDLK_a: joypad->release_joypad_button(Joypad::BUTTON::LEFT);   break;
            case SDLK_s: joypad->release_joypad_button(Joypad::BUTTON::DOWN);   break;
            case SDLK_d: joypad->release_joypad_button(Joypad::BUTTON::RIGHT);  break;
            case SDLK_z: joypad->release_joypad_button(Joypad::BUTTON::A);      break;
            case SDLK_x: joypad->release_joypad_button(Joypad::BUTTON::B);      break;
            case SDLK_m: joypad->release_joypad_button(Joypad::BUTTON::START);  break;
            case SDLK_n: joypad->release_joypad_button(Joypad::BUTTON::SELECT); break;
            }
            break;
        }

        //joypadx->refreshButtonStates(0);

        } // switch()

        std::this_thread::sleep_for(std::chrono::microseconds(200));

    } // end while(run)

    // Close emulator thread
    thread.join();

    return 0;
}