#include <SDLWindow.h>
#include <algorithm>

SDLWindow::SDLWindow()
{
    init();
}

SDLWindow::~SDLWindow()
{
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void SDLWindow::init()
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);

    window = SDL_CreateWindow("GBCEmulator",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_PIXEL_W * 4, SCREEN_PIXEL_H * 4,
        SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);

    glContext = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);

    SDL_SetRenderDrawColor(renderer, 100, 255, 255, 255);
    screen_texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_PIXEL_W,
        SCREEN_PIXEL_H);

    screen_texture_rect = { 0, 0, SCREEN_PIXEL_W * 4, SCREEN_PIXEL_H * 4 };
}

void SDLWindow::hookToEmulator(std::shared_ptr<GBCEmulator> emulator)
{
    if (!emulator)
    {
        return;
    }

    if (emu != emulator)
    {
        emu = emulator;
    }

    // Set emulator display output to SDL screen
    emulator->setFrameUpdateMethod(std::bind(&SDLWindow::display, this, std::placeholders::_1));

    // Get emulator joypad, hook up XInput joypad to emulator joypad
    joypad = emulator->get_Joypad();
    joypadx = std::make_shared<JoypadXInput>(joypad);   // Joypad XInput support
}

void SDLWindow::display(SDL_Color * frame)
{
    SDL_UpdateTexture(screen_texture, NULL, frame, SCREEN_PIXEL_W * sizeof(SDL_Color));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, screen_texture, NULL, &screen_texture_rect);
    SDL_RenderPresent(renderer);
}

void SDLWindow::updateWindowTitle(const uint64_t & framerate)
{
    if (window)
    {
        const std::string title = "GBCEmulator | " + std::to_string(framerate);
        SDL_SetWindowTitle(window, title.c_str());
    }
}

int SDLWindow::run()
{
    SDL_Event event;
    bool run = true;

    while (run)
    {   // Process input here
        SDL_PollEvent(&event);
        switch (event.type)
        {
        case SDL_QUIT:
        {
            run = false;
            break;
        }
        case SDL_DROPFILE:
        {
            char* romName = event.drop.file;
            const std::string romNameStr(romName);

            // Check file extension for valid game type
            if (romIsValid(romNameStr))
            {
                emu = std::make_shared<GBCEmulator>(romNameStr, romNameStr + ".log");
                hookToEmulator(emu);
                startEmulator();
            }

            // Free file hold
            SDL_free(romName);
        }
        case SDL_KEYDOWN:
        {
            switch (event.key.keysym.sym)
            {
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
        } // end case SDL_KEYDOWN

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
        } // end case SDL_KEYUP

        case SDL_WINDOWEVENT:
        {
            switch (event.window.event)
            {
            case SDL_WINDOWEVENT_SIZE_CHANGED:
                //emu->resizeSDLRenderWindow(event.window.data1, event.window.data2);
                break;
            }
        }

        } // switch(event.type)

        if (joypadx)
        {
            joypadx->refreshButtonStates(joypadx->findControllers());
        }

        std::this_thread::sleep_for(std::chrono::microseconds(200));

    } // end while(run)

    if (emu)
    {   // Stop the emulator
        emu->stop();
    }

    if (emu_thread.joinable())
    {
        // Close emulator thread
        emu_thread.join();
    }

    return 0;
}

void SDLWindow::startEmulator()
{
    if (!emu)
    {
        return;
    }

    if (emu_thread.joinable())
    {
        emu_thread.join();
    }

    // Have emulator tick in its own thread
    emu_thread = std::thread([&]()
    {
        emu->run();
    });
}

std::string SDLWindow::getFileExtension(const std::string& filepath)
{
    std::string ret = "";
    size_t index;
    if (index = filepath.find_last_of(".") &&
        index != std::string::npos)
    {   // Found at least one "."
        ret = filepath.substr(index + 1);
    }

    return ret;
}

bool SDLWindow::romIsValid(const std::string& filepath)
{
    bool ret = false;
    std::string fileExtension = getFileExtension(filepath);

    // Make file extension lowercase
    std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);

    if (fileExtension.rfind(".gb") ||
        fileExtension.rfind(".gbc"))
    {
        ret = true;
    }

    return ret;
}