#include <SDLWindow.h>
#include <algorithm>
#include <SDL_thread.h>

<<<<<<< HEAD
SDLWindow::SDLWindow(const std::string& log_name)
    :   ScreenInterface()
    , logger(spdlog::rotating_logger_mt("SDLWindow", log_name, 1024 * 1024 * 3, 3))
=======
SDLWindow::SDLWindow()
    : ScreenInterface()
    , keep_aspect_ratio(true)
>>>>>>> qt_gui
{
    init();

    std::array<SDL_Color, SCREEN_PIXEL_TOTAL> grayFrame;
    for (SDL_Color& pixel : grayFrame)
    {
        pixel.r = pixel.g = pixel.b = 200;
    }
    display(grayFrame);
}

SDLWindow::~SDLWindow()
{
    std::lock_guard<std::mutex> lg(renderer_mutex);
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void SDLWindow::init()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        logger->error("SDL_Init() failed: {}", SDL_GetError());
    }

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
<<<<<<< HEAD
        SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window)
    {
        logger->error("SDL_CreateWindow() failed: {}", SDL_GetError());
    }
=======
        SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL |
        SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
>>>>>>> qt_gui

    glContext = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(1); // Enable vsync
    if (!glContext)
    {
        logger->error("SDL_GL_CreateContext() failed: {}", SDL_GetError());
    }

    renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
    if (!renderer)
    {
        logger->error("SDL_CreateRenderer() failed: {}", SDL_GetError());
    }

    if (keep_aspect_ratio)
    {   // Force original aspect ratio
        SDL_RenderSetLogicalSize(renderer, SCREEN_PIXEL_W, SCREEN_PIXEL_H);
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    screen_texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_PIXEL_W,
        SCREEN_PIXEL_H);
    if (!screen_texture)
    {
        logger->error("SDL_CreateTexture() failed: {}", SDL_GetError());
    }

    screen_texture_rect = { 0, 0, SCREEN_PIXEL_W * 4, SCREEN_PIXEL_H * 4 };

    //SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, reinterpret_cast<char*>(SDLRenderType::NEAREST_PIXEL));
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
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

void SDLWindow::display(std::array<SDL_Color, SCREEN_PIXEL_TOTAL> frame)
{
    if (emu)
    {
        updateWindowTitle(std::to_string(emu->frameShowTimeMicro.count()));    // Turn microseconds into milliseconds
    }

    if (!renderer)
    {
        return;
    }

    std::lock_guard<std::mutex> lg(renderer_mutex);
    SDL_UpdateTexture(screen_texture, NULL, frame.data(), SCREEN_PIXEL_W * sizeof(SDL_Color));
    //SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void SDLWindow::updateWindowTitle(const std::string & framerate)
{
    if (window)
    {
        const std::string title = "GBCEmulator | "
            + emu->getGameTitle()
            + " | "
            + framerate;
        SDL_SetWindowTitle(window, title.c_str());
    }
}

int SDLWindow::run(bool start_emu)
{
    SDL_Event event;
    bool run = true;

    if (start_emu)
    {   // Already hooked up emulator, start it on run()
        startEmulator();
    }

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
                if (emu)
                {
                    emu->stop();
                }

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
            case SDLK_r:
            {
                loadSaveState();
                break;
            }
            case SDLK_t:
            {
                takeSaveState();
                break;
            }
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
                std::lock_guard<std::mutex> lg(renderer_mutex);
                // Clear first frame in double buffer
                SDL_RenderClear(renderer);
                SDL_RenderPresent(renderer);
                // Clear second frame in double buffer
                SDL_RenderClear(renderer);
                SDL_RenderPresent(renderer);
                break;
            }
        }

        } // switch(event.type)

        if (joypadx)
        {
            //joypadx->refreshButtonStates(joypadx->findControllers());
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

    int ret = SDL_SetThreadPriority(SDL_ThreadPriority::SDL_THREAD_PRIORITY_TIME_CRITICAL);

    // Have emulator tick in its own thread
    emu_thread = std::thread([&]()
    {
        int ret = SDL_SetThreadPriority(SDL_ThreadPriority::SDL_THREAD_PRIORITY_TIME_CRITICAL);
        emu->run();
    });
}

std::string SDLWindow::getFileExtension(const std::string& filepath)
{
    std::string ret = "";
    size_t index = filepath.find_last_of(".");
    if (index != std::string::npos)
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

void SDLWindow::takeSaveState()
{
    if (!emu)
    {
        return;
    }

    // Stop the emulator first so we don't save it while running
    emu->setStopRunning(true);
    if (emu_savestate)
    {
        emu_savestate.reset();
    }

    // Create emulator savestate
    emu_savestate = std::make_shared<GBCEmulator>(emu->getROMName(), emu->getROMName() + ".log");

    // Copy current emulator into emulator savestate
    *emu_savestate.get() = *emu.get();

    // Start emulator again
    startEmulator();
}

void SDLWindow::loadSaveState()
{
    if (!emu_savestate || !emu)
    {   // Need a save state and an emulator to load a savestate
        return;
    }

    emu->stop();
    if (emu_thread.joinable())
    {
        // Close emulator thread
        emu_thread.join();
    }

    // Load emulator savestate into emulator
    *emu.get() = *emu_savestate.get();

    hookToEmulator(emu);
    startEmulator();
}