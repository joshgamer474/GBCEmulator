#include <SDLWindow.h>
#include <SDL3/SDL_thread.h>
#include <SDL3/SDL_stdinc.h>

#include <algorithm>

#include <fmt/core.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <Util.h>

SDLWindow::SDLWindow(const std::string& log_name)
    :   ScreenInterface()
    , keep_aspect_ratio(true)
    , have_new_frame(false)
    , using_connected_controller(-1)
{
    // Only log out to file if not on Apple
#if __APPLE__
    logger = spdlog::stdout_color_mt("console");
#else
    const std::string logFile = generate_log_path("SDLWindow");
    logger = spdlog::rotating_logger_mt(logFile, log_name, 1024 * 1024 * 3, 3);
#endif

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
    SDL_GL_DestroyContext(glContext);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    logger->flush();
}

void SDLWindow::init()
{
    logger->set_level(spdlog::level::info);
    logger->info("Started init()");

    //SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO) == false)
    {
        logger->error("SDL_Init() failed: {}", SDL_GetError());
        return;
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    const SDL_DisplayMode* current = SDL_GetCurrentDisplayMode(0);

    window = SDL_CreateWindow("GBCEmulator",
        SCREEN_PIXEL_W * 4, SCREEN_PIXEL_H * 4,
        SDL_WINDOW_OPENGL |
        SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        logger->error("SDL_CreateWindow() failed: {}", SDL_GetError());
    }

    glContext = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(1); // Enable vsync
    if (!glContext)
    {
        logger->error("SDL_GL_CreateContext() failed: {}", SDL_GetError());
    }

    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer)
    {
        logger->error("SDL_CreateRenderer() failed: {}", SDL_GetError());
    }

    if (keep_aspect_ratio)
    {   // Force original aspect ratio
        SDL_SetRenderLogicalPresentation(renderer, SCREEN_PIXEL_W, SCREEN_PIXEL_H, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
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
    SDL_SetTextureScaleMode(screen_texture, SDL_SCALEMODE_NEAREST);

    screen_texture_rect = { 0, 0, SCREEN_PIXEL_W * 4, SCREEN_PIXEL_H * 4 };

    logger->info("init() complete");
}

void SDLWindow::hookToEmulator(std::shared_ptr<GBCEmulator> emulator)
{
    logger->info("Hooking up new emulator to SDLWindow");

    if (!emulator)
    {
        logger->error("Refusing to hook to empty emulator");
        return;
    }

    if (emu != emulator)
    {
        emu = emulator;
        updateWindowTitle("");
    }

    // Set emulator display output to SDL screen
    emulator->setFrameUpdateMethod(std::bind(&SDLWindow::display, this, std::placeholders::_1));

    // Get emulator joypad, hook up XInput joypad to emulator joypad
    joypad = emulator->get_Joypad();
    joypadx = std::make_shared<JoypadXInput>(joypad);   // Joypad XInput support
}

void SDLWindow::display(std::array<SDL_Color, SCREEN_PIXEL_TOTAL> frame)
{
    if (!renderer)
    {
        logger->error("Refusing to display() without renderer");
        return;
    }

    std::lock_guard<std::mutex> lg(renderer_mutex);
    curr_frame = frame;
    have_new_frame = true;
}

void SDLWindow::updateWindowTitle(const std::string & framerate)
{
    if (window)
    {
        std::string title = "GBCEmulator | "
            + emu->getGameTitle();
        if (!framerate.empty())
        {
            title += " | " + framerate;
        }
        SDL_SetWindowTitle(window, title.c_str());
    }
}

int SDLWindow::run(bool start_emu)
{
    SDL_Event event;
    bool run = true;

    logger->info("run(), start_emu: {}", start_emu);

    if (start_emu)
    {   // Already hooked up emulator, start it on run()
        startEmulator();
    }

    while (run)
    {   // Process input here
        SDL_PollEvent(&event);
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
        {
            run = false;
            break;
        }
        case SDL_EVENT_DROP_FILE:
        {
            const char* romName = event.drop.data;
            const std::string romNameStr(romName);
            const std::string biosPath = 
                //"/home/childers/Downloads/bios.gbc";
                //"/home/childers/Downloads/bios.gb";
                "";

            // Check file extension for valid game type
            if (romIsValid(romNameStr))
            {
                if (emu)
                {
                    emu->stop();
                }

                // Generate a log file name based on OS
                const std::string logFile = generate_log_path(romNameStr);

                // Start emulator
                emu = std::make_shared<GBCEmulator>(romNameStr,
                    logFile,
                    biosPath,
                    false,      // Debug mode
                    false);     // Force CGB mode
                hookToEmulator(emu);
                updateWindowTitle("");
                startEmulator();
            }

            // Free file hold
            //SDL_free(romName);
        }
        case SDL_EVENT_KEY_DOWN:
        {
            switch (event.key.key)
            {
            case SDLK_W: joypad->set_joypad_button(Joypad::BUTTON::UP);     break;
            case SDLK_A: joypad->set_joypad_button(Joypad::BUTTON::LEFT);   break;
            case SDLK_S: joypad->set_joypad_button(Joypad::BUTTON::DOWN);   break;
            case SDLK_D: joypad->set_joypad_button(Joypad::BUTTON::RIGHT);  break;
            case SDLK_Z: joypad->set_joypad_button(Joypad::BUTTON::A);      break;
            case SDLK_X: joypad->set_joypad_button(Joypad::BUTTON::B);      break;
            case SDLK_M: joypad->set_joypad_button(Joypad::BUTTON::START);  break;
            case SDLK_N: joypad->set_joypad_button(Joypad::BUTTON::SELECT); break;
            case SDLK_R:
            {
                loadSaveState();
                break;
            }
            case SDLK_T:
            {
                takeSaveState();
                break;
            }
            case SDLK_O:
            {
                emu->changeCGBPalette();
                break;
            }
            } // end switch()
            break;
        } // end case SDL_KEYDOWN

        case SDL_EVENT_KEY_UP:
        {
            switch (event.key.key)
            {
            case SDLK_W: joypad->release_joypad_button(Joypad::BUTTON::UP);     break;
            case SDLK_A: joypad->release_joypad_button(Joypad::BUTTON::LEFT);   break;
            case SDLK_S: joypad->release_joypad_button(Joypad::BUTTON::DOWN);   break;
            case SDLK_D: joypad->release_joypad_button(Joypad::BUTTON::RIGHT);  break;
            case SDLK_Z: joypad->release_joypad_button(Joypad::BUTTON::A);      break;
            case SDLK_X: joypad->release_joypad_button(Joypad::BUTTON::B);      break;
            case SDLK_M: joypad->release_joypad_button(Joypad::BUTTON::START);  break;
            case SDLK_N: joypad->release_joypad_button(Joypad::BUTTON::SELECT); break;
            }
            break;
        } // end case SDL_KEYUP

        case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
        {
            SDL_Log("JOYBUTTONDOWN %d", event.jbutton.button);

            switch (event.jbutton.button)
            {
                case SDL_GAMEPAD_BUTTON_SOUTH:           emu->set_joypad_button(Joypad::BUTTON::A); break;
                case SDL_GAMEPAD_BUTTON_EAST:           emu->set_joypad_button(Joypad::BUTTON::B); break;
                case SDL_GAMEPAD_BUTTON_START:       emu->set_joypad_button(Joypad::BUTTON::START); break;
                case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:emu->set_joypad_button(Joypad::BUTTON::SELECT); break;
                case SDL_GAMEPAD_BUTTON_DPAD_UP:     emu->set_joypad_button(Joypad::BUTTON::UP); break;
                case SDL_GAMEPAD_BUTTON_DPAD_DOWN:   emu->set_joypad_button(Joypad::BUTTON::DOWN); break;
                case SDL_GAMEPAD_BUTTON_DPAD_LEFT:   emu->set_joypad_button(Joypad::BUTTON::LEFT); break;
                case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:  emu->set_joypad_button(Joypad::BUTTON::RIGHT); break;
                case SDL_GAMEPAD_BUTTON_BACK:
                {
                    //quit = true;
                    break;
                }
            }
            break;
        }

        case SDL_EVENT_JOYSTICK_BUTTON_UP:
        {
            SDL_Log("JOYBUTTONUP %d", event.jbutton.button);

            switch (event.jbutton.button)
            {
                case SDL_GAMEPAD_BUTTON_SOUTH:       emu->release_joypad_button(Joypad::BUTTON::A); break;
                case SDL_GAMEPAD_BUTTON_EAST:        emu->release_joypad_button(Joypad::BUTTON::B); break;
                case SDL_GAMEPAD_BUTTON_START:       emu->release_joypad_button(Joypad::BUTTON::START); break;
                case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:emu->release_joypad_button(Joypad::BUTTON::SELECT); break;
                case SDL_GAMEPAD_BUTTON_DPAD_UP:     emu->release_joypad_button(Joypad::BUTTON::UP); break;
                case SDL_GAMEPAD_BUTTON_DPAD_DOWN:   emu->release_joypad_button(Joypad::BUTTON::DOWN); break;
                case SDL_GAMEPAD_BUTTON_DPAD_LEFT:   emu->release_joypad_button(Joypad::BUTTON::LEFT); break;
                case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:  emu->release_joypad_button(Joypad::BUTTON::RIGHT); break;
                case SDL_GAMEPAD_BUTTON_BACK:
                {

                }
            }
            break;
        }

        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        {
            std::lock_guard<std::mutex> lg(renderer_mutex);
            // Clear first frame in double buffer
            SDL_RenderClear(renderer);
            SDL_RenderPresent(renderer);
            // Clear second frame in double buffer
            SDL_RenderClear(renderer);
            SDL_RenderPresent(renderer);
            break;
        }

        } // switch(event.type)

        if (joypadx)
        {   // Check if a controller has been selected/found yet
            if (using_connected_controller < 0)
            {   // Select new controller to use
                using_connected_controller = joypadx->findControllers();
            }
            joypadx->refreshButtonStates(using_connected_controller);
        }

        if (have_new_frame)
        {
            std::lock_guard<std::mutex> lg(renderer_mutex);
            SDL_UpdateTexture(screen_texture, NULL, curr_frame.data(), SCREEN_PIXEL_W * sizeof(SDL_Color));
            //SDL_RenderClear(renderer);
            SDL_RenderTexture(renderer, screen_texture, NULL, NULL);
            SDL_RenderPresent(renderer);
            have_new_frame = false;
#ifndef __ANDROID__
            if (emu)
            {
                updateWindowTitle(fmt::format("{:.2f}", emu->frameShowTimeMicro.count() / 1000.0));    // Turn microseconds into milliseconds
            }
#endif
        }

        std::this_thread::sleep_for(std::chrono::microseconds(200));
    } // end while(run)

    logger->info("Left while() loop in run()");

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
        logger->error("Refusing to start non existant emulator");
        return;
    }

    if (emu_thread.joinable())
    {
        emu_thread.join();
    }

    int ret = SDL_SetCurrentThreadPriority(SDL_ThreadPriority::SDL_THREAD_PRIORITY_TIME_CRITICAL);

    // Have emulator tick in its own thread
    logger->info("Starting emulator thread");
    emu_thread = std::thread([&]()
    {
        int ret = SDL_SetCurrentThreadPriority(SDL_ThreadPriority::SDL_THREAD_PRIORITY_TIME_CRITICAL);
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
        fileExtension.rfind(".gbc") ||
        fileExtension.rfind(".zip"))
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