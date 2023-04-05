//
// Created by childers on 6/9/19.
//

#include <SDLWindowAndroid.h>
#include <algorithm>
#include <fstream>
#include <vector>
#include <SDL_thread.h>
#include <SDL2/SDL_image.h>

SDLWindowAndroid::SDLWindowAndroid(const std::string& config_path,
        const std::string& log_name,
        const BGColor background_color)
        :   ScreenInterface()
        , logger(spdlog::rotating_logger_mt("SDLWindowAndroid", log_name, 1024 * 1024 * 3, 3))
        , config_path(config_path)
        , keep_aspect_ratio(true)
        , initialized_SDL(false)
        , new_frame(false)
        , quit(false)
        , is_android_tv(false)
        , num_joypads_looked_at(0)
        , sdl_joystick(nullptr)
        , bg_color(background_color)
        , screen_orientation_changed(false)
{
    SDL_Log("Created SDLWindow");
    init();

    loadSDLTextureRects();

    std::array<SDL_Color, SCREEN_PIXEL_TOTAL> grayFrame;
    for (SDL_Color& pixel : grayFrame)
    {
        pixel.r = pixel.g = pixel.b = 200;
    }
    display(grayFrame);
}

SDLWindowAndroid::~SDLWindowAndroid()
{
    SDL_Log("Starting to destruct SDLWindowAndroid");

    quit = true;

    // Close emulator
    stopEmulator();

    cleanupSDL();

    saveSDLTextureRects();

    logger->info("Finished destructing SDLWindowAndroid");
    logger->flush();
    spdlog::shutdown();

    SDL_Log("Done destructing SDL");
}

void SDLWindowAndroid::cleanupSDL()
{
    std::lock_guard<std::mutex> lg(renderer_mutex);

    // Close opened joysticks
    for (auto& [key, val] : joystickMap)
    {
        SDL_JoystickClose(val);
    }
    joystickMap.clear();

    for (SDLTextureRect* textureRect : buttonTextures)
    {
        if (textureRect && textureRect->texture)
        {
            SDL_DestroyTexture(textureRect->texture);
            textureRect->texture = nullptr;
        }
    }

    if (sdl_joystick &&
        SDL_JoystickGetAttached(sdl_joystick))
    {
        SDL_JoystickClose(sdl_joystick);
    }

    if (glContext)
    {
        SDL_GL_DeleteContext(glContext);
        glContext = nullptr;
    }

    if (renderer)
    {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }

    if (window)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    SDL_Quit();
}

void SDLWindowAndroid::init()
{
    logger->set_level(spdlog::level::info);
    logger->info("Started init()");
    SDL_Log("Started init()");

    //SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) != 0)
    {
        SDL_Log("SDL_Init() failed: %s", SDL_GetError());
        logger->error("SDL_Init() failed: {}", SDL_GetError());
        return;
    }

    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_INFO);
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);

    //is_android_tv = SDL_IsAndroidTV();

    SDL_Log("Setting GL attributes");
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

    SDL_GetCurrentDisplayMode(0, &current_display_mode);
    current_display_mode.w /= 4;
    current_display_mode.h /= 4;
    SDL_Log("Width: %d, Height: %d",current_display_mode.w, current_display_mode.h);


    SDL_Log("Doing SDL_CreateWindow()");
    window = SDL_CreateWindow("GBCEmulator",
                              0,
                              0,
                              //SCREEN_PIXEL_W, SCREEN_PIXEL_H,
                              current_display_mode.w, current_display_mode.h,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL |
                              SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        SDL_Log("SDL_CreateWindow() failed: %s", SDL_GetError());
        logger->error("SDL_CreateWindow() failed: {}", SDL_GetError());
        return;
    }

    SDL_Log("Doing SDL_GL_CreateContext()");
    glContext = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(1); // Enable vsync
    if (!glContext)
    {
        SDL_Log("SDL_GL_CreateContext() failed");
        logger->error("SDL_GL_CreateContext() failed: {}", SDL_GetError());
        return;
    }

    SDL_Log("Doing SDL_CreateRenderer()");
    renderer = SDL_CreateRenderer(window, -1,
      SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
    if (!renderer)
    {
        SDL_Log("SDL_GL_CreateRenderer() failed");
        logger->error("SDL_CreateRenderer() failed: {}", SDL_GetError());
        return;
    }

    if (keep_aspect_ratio)
    {   // Force original aspect ratio
        //SDL_RenderSetLogicalSize(renderer, SCREEN_PIXEL_W, SCREEN_PIXEL_H);
    }

    SDL_Log("Doing SDL_CreateTexture()");
    const RGB bg_rgb = GetColor(bg_color);
    SDL_SetRenderDrawColor(renderer, bg_rgb.r, bg_rgb.g, bg_rgb.b, 255);
    screen_texture_rect.texture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_RGBA32,
            SDL_TEXTUREACCESS_STREAMING,
            SCREEN_PIXEL_W, SCREEN_PIXEL_H);

    if (!screen_texture_rect.texture)
    {
        SDL_Log("SDL_CreateTexture() failed");
        logger->error("SDL_CreateTexture() failed: {}", SDL_GetError());
        return;
    }

    if (keep_aspect_ratio)
    {
        int scaleMultiplier = 1;

        const bool is_landscape = current_display_mode.w > current_display_mode.h;

        // Make texture size based on screen size
        if (is_android_tv ||
            is_landscape)
        {   // scale to height (LANDSCAPE) (e.g. TV)
            scaleMultiplier = current_display_mode.h / SCREEN_PIXEL_H;
        }
        else
        {   // scale to width (PORTRAIT) (e.g. phone)
            scaleMultiplier = current_display_mode.w / SCREEN_PIXEL_W;
        }

        const int textureWidth = SCREEN_PIXEL_W * scaleMultiplier;
        const int textureHeight = SCREEN_PIXEL_H * scaleMultiplier;
        const int xBorder = (current_display_mode.w - textureWidth) / 2;
        const int yBorder = (is_android_tv || is_landscape) ? ((current_display_mode.h - textureHeight) / 2) : xBorder;

        screen_texture_rect.rect = {
                xBorder, yBorder,
                textureWidth,
                textureHeight};
    }
    else
    {   // Scale texture to fit width of phone
        const float scaleMultiplierFloat = current_display_mode.w / (SCREEN_PIXEL_W * 1.0f);
        const int textureHeight = SCREEN_PIXEL_H * scaleMultiplierFloat;
        screen_texture_rect.rect = {
                0, 0,
                current_display_mode.w, textureHeight};
    }

    if (!is_android_tv)
    {   // Create Gamepad textures
        initGamepadTextures();
    }

    //SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, reinterpret_cast<char*>(SDLRenderType::NEAREST_PIXEL));
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_SetHintWithPriority(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0", SDL_HintPriority::SDL_HINT_OVERRIDE);

    // Update variables
    move_buttons = false;
    pressedDPAD = false;
    backIsPressed = false;
    backPressed = 0;
    numEventsPerFrame = 0;
    fps = 0;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
    SDL_GL_GetDrawableSize(window, &drawableWidth, &drawableHeight);

    buttonTextures  = {
            &texture_button_a,
            &texture_button_b,
            &texture_button_start,
            &texture_button_select,
            &texture_button_dpad_border,
            &texture_button_dpad,
            &texture_button_bgcolor_change,
            &texture_button_dmgpalette_change
    };

    SDL_Log("ini() complete");
    logger->info("init() complete");
    initialized_SDL = true;
}

void SDLWindowAndroid::initGamepadTextures()
{
    // Create Gamepad texture
    SDL_Log("Doing SDL_CreateTexture()");
    gamepad_texture_rect.texture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_RGBA32,
            SDL_TEXTUREACCESS_STREAMING,
            current_display_mode.w,
            current_display_mode.h);
    if (!gamepad_texture_rect.texture)
    {
        SDL_Log("SDL_CreateTexture() failed: %s", SDL_GetError());
        logger->error("SDL_CreateTexture() failed: {}", SDL_GetError());
        return;
    }
    SDL_SetTextureColorMod(gamepad_texture_rect.texture, 0, 255, 0);
    gamepad_texture_rect.rect = { 0, SCREEN_PIXEL_H,
                                  current_display_mode.w, current_display_mode.h - SCREEN_PIXEL_H};

    // Load Gamepad button textures
    loadImageIntoTextureRect("button_A.png", texture_button_a);
    loadImageIntoTextureRect("button_B.png", texture_button_b);
    loadImageIntoTextureRect("button_START.png", texture_button_start);
    loadImageIntoTextureRect("button_SELECT.png", texture_button_select);
    loadImageIntoTextureRect("button_DPAD_CIRCLE.png", texture_button_dpad);
    loadImageIntoTextureRect("button_DPAD_CIRCLE_BORDER.png", texture_button_dpad_border);
    loadImageIntoTextureRect("button_DPAD_CIRCLE_BORDER.png", texture_button_bgcolor_change);
    loadImageIntoTextureRect("button_DPAD_CIRCLE_BORDER.png", texture_button_dmgpalette_change);

    resetButtons();
}

void SDLWindowAndroid::resetButtons()
{
    // Base placement of buttons based on remaining space left
    const int height_used = screen_texture_rect.rect.h + screen_texture_rect.rect.y;

    resetTextureSize(texture_button_bgcolor_change);
    resetTextureSize(texture_button_dmgpalette_change);
    resetTextureSize(texture_button_a);
    resetTextureSize(texture_button_b);
    resetTextureSize(texture_button_start);
    resetTextureSize(texture_button_select);
    resetTextureSize(texture_button_dpad);

    // BG Color change
    texture_button_bgcolor_change.rect.w *= 0.25;
    texture_button_bgcolor_change.rect.h *= 0.25;
    texture_button_bgcolor_change.rect.x = (current_display_mode.w * 0.5) - (texture_button_bgcolor_change.rect.w >> 1);
    texture_button_bgcolor_change.rect.y = height_used + (current_display_mode.h * 0.05);

    // DMG Palette Change
    texture_button_dmgpalette_change.rect.w *= 0.18;
    texture_button_dmgpalette_change.rect.h *= 0.18;
    texture_button_dmgpalette_change.rect.x =
            (texture_button_bgcolor_change.rect.x + (texture_button_bgcolor_change.rect.w * 0.5))
            - (texture_button_dmgpalette_change.rect.w * 0.5);
    texture_button_dmgpalette_change.rect.y =
            texture_button_bgcolor_change.rect.y + texture_button_bgcolor_change.rect.h
            + (texture_button_dmgpalette_change.rect.h * 0.5);

    // A
    texture_button_a.rect.x = current_display_mode.w * 0.75;
    texture_button_a.rect.y = height_used + (current_display_mode.h * 0.15);
    texture_button_a.joypad_button = Joypad::BUTTON::A;

    // B
    texture_button_b.rect.x = texture_button_a.rect.x - texture_button_b.rect.w;
    texture_button_b.rect.y = texture_button_a.rect.y + texture_button_b.rect.h;
    texture_button_b.joypad_button = Joypad::BUTTON::B;

    // START (right)
    texture_button_start.rect.w *= 0.5;
    texture_button_start.rect.h *= 0.5;
    texture_button_start.rect.x = (current_display_mode.w * 0.5) + (texture_button_start.rect.w >> 2);
    texture_button_start.rect.y = texture_button_b.rect.y + (texture_button_b.rect.h * 1.5);
    texture_button_start.joypad_button = Joypad::BUTTON::START;

    // SELECT (left)
    texture_button_select.rect.w *= 0.5;
    texture_button_select.rect.h *= 0.5;
    texture_button_select.rect.x = texture_button_start.rect.x - (texture_button_select.rect.w * 1.5);
    texture_button_select.rect.y = texture_button_b.rect.y + (texture_button_b.rect.h * 1.5);
    texture_button_select.joypad_button = Joypad::BUTTON::SELECT;

    // DPAD
    texture_button_dpad.rect.x = current_display_mode.w * 0.15;
    texture_button_dpad.rect.y = texture_button_b.rect.y - (texture_button_b.rect.y - texture_button_a.rect.y);
    dpad.initialize(texture_button_dpad.rect);

    texture_button_dpad_border.rect = dpad.bounding_rect;
}

void SDLWindowAndroid::hookToEmulator(std::shared_ptr<GBCEmulator> emulator)
{
    SDL_Log("Hooking up new emulator to SDLWindowAndroid");
    logger->info("Hooking up new emulator to SDLWindowAndroid");

    if (!emulator)
    {
        SDL_Log("Refusing to hook to empty emulator");
        logger->error("Refusing to hook to empty emulator");
        return;
    }

    if (emu != emulator)
    {
        emu = emulator;
    }

    // Set emulator display output to SDL screen
    emulator->setFrameUpdateMethod(std::bind(&SDLWindowAndroid::display, this, std::placeholders::_1));

    // Get emulator joypad, hook up XInput joypad to emulator joypad
    joypad = emulator->get_Joypad();
    //joypadx = std::make_shared<JoypadXInput>(joypad);   // Joypad XInput support
}

void SDLWindowAndroid::display(std::array<SDL_Color, SCREEN_PIXEL_TOTAL> frame)
{
    if (!renderer)
    {
        logger->error("Refusing to display() without renderer");
        return;
    }

    //SDL_Log("Updating display with new frame");
    curr_frame = frame;
    new_frame = true;
}

void SDLWindowAndroid::updateWindowTitle(const std::string & framerate)
{
    if (window)
    {
        //SDL_Log("Updating window title");

        const std::string title = "GBCEmulator | "
                                  + emu->getGameTitle()
                                  + " | "
                                  + framerate;
        SDL_SetWindowTitle(window, title.c_str());
    }
}

int SDLWindowAndroid::run(bool start_emu)
{
    SDL_Event event;
    bool run = true;
    int x, y;
    pressedDPAD = false;
    numEventsPerFrame = 0;
    uint32_t microSleep = 500;
    backPressed = 0;
    backIsPressed = false;

    if (!initialized_SDL)
    {
        SDL_Log("Returning -1 from run() as SDL is not initialized");
        return -1;
    }

    logger->info("run(), start_emu: {}", start_emu);

    if (start_emu)
    {   // Already hooked up emulator, start it on run()
        startEmulator();
    }

    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
    SDL_GL_GetDrawableSize(window, &drawableWidth, &drawableHeight);

    while (run && !quit)
    {   // Process input here
        update(run);
        std::this_thread::sleep_for(std::chrono::microseconds(microSleep));
    } // end while(run)

    logger->info("Left while() loop in run()");

    stopEmulator();

    return 0;
}

void SDLWindowAndroid::handleSDLEvent(SDL_Event& event, bool& run)
{
    int x, y;
    SDL_Joystick* controller = nullptr;

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

        case SDL_JOYDEVICEADDED:
        {
            controller = SDL_JoystickOpen(event.jdevice.which);
            const std::string joystickName = std::string(SDL_JoystickNameForIndex(event.jdevice.which));

            SDL_Log("Joystick %d,"
                    "name: %s, number of axes: %d, number of buttons %d",
                    event.jdevice.which,
                    SDL_JoystickNameForIndex(event.jdevice.which),
                    SDL_JoystickNumAxes(controller),
                    SDL_JoystickNumButtons(controller));

            // Check if controller is Accelerometer
            if (joystickName.find("Accelerometer") != std::string::npos)
            {   // Close controller
                SDL_Log("Refusing to open Accelerometer as a controller");
                SDL_JoystickClose(controller);
                break;
            }

            // Add device to joystick map
            joystickMap[event.jdevice.which] = controller;
            SDL_Log("SDL_JOYDEVICEADDED");
            break;
        }

        case SDL_JOYDEVICEREMOVED:
        {
            SDL_Log("SDL_JOYDEVICEREMOVED");
            if (joystickMap.find(event.jdevice.which) != joystickMap.end())
            {
                SDL_Log("Closing joystick");
                SDL_JoystickClose(joystickMap[event.jdevice.which]);
            }
            break;
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
                case SDLK_AC_BACK:
                {
                    //SDL_AndroidBackButton();
                    backIsPressed = true;
                    //quit = true;
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
                case SDLK_AC_BACK:
                {
                    if (backIsPressed)
                    {
                        backIsPressed = false;
                        backPressed++;
                        SDL_Log("Back Pressed: %d", backPressed);
                    }
                    break;
                }
            }
            break;
        } // end case SDL_KEYUP

        case SDL_JOYAXISMOTION:
        {
            switch (event.jaxis.axis)
            {
                case SDL_CONTROLLER_AXIS_LEFTX:
                {
                    if (event.jaxis.value > -8000 &&
                        event.jaxis.value < 8000)
                    {   // Have some padding in the joysticks
                        joypad->release_joypad_button(Joypad::BUTTON::LEFT);
                        joypad->release_joypad_button(Joypad::BUTTON::RIGHT);
                        break;
                    }

                    if (event.jaxis.value < 0)
                    {   // Left?
                        joypad->release_joypad_button(Joypad::BUTTON::RIGHT);
                        joypad->set_joypad_button(Joypad::BUTTON::LEFT);
                    }
                    else
                    {   // Right?
                        joypad->release_joypad_button(Joypad::BUTTON::LEFT);
                        joypad->set_joypad_button(Joypad::BUTTON::RIGHT);
                    }
                    break;
                }
                case SDL_CONTROLLER_AXIS_LEFTY:
                {
                    if (event.jaxis.value > -8000 &&
                        event.jaxis.value < 8000)
                    {   // Have some padding in the joysticks
                        joypad->release_joypad_button(Joypad::BUTTON::DOWN);
                        joypad->release_joypad_button(Joypad::BUTTON::UP);
                        break;
                    }

                    if (event.jaxis.value < 0)
                    {   // Up?
                        joypad->release_joypad_button(Joypad::BUTTON::DOWN);
                        joypad->set_joypad_button(Joypad::BUTTON::UP);
                    }
                    else
                    {   // Down?
                        joypad->release_joypad_button(Joypad::BUTTON::UP);
                        joypad->set_joypad_button(Joypad::BUTTON::DOWN);
                    }
                    break;
                }
            }
            break;
        }

        case SDL_JOYBUTTONDOWN:
        {
            SDL_Log("JOYBUTTON DOWN %d", event.jbutton.button);

            switch (event.jbutton.button)
            {
                case SDL_CONTROLLER_BUTTON_A:
                case SDL_CONTROLLER_BUTTON_Y:           joypad->set_joypad_button(Joypad::BUTTON::A); break;
                case SDL_CONTROLLER_BUTTON_B:
                case SDL_CONTROLLER_BUTTON_X:           joypad->set_joypad_button(Joypad::BUTTON::B); break;
                case SDL_CONTROLLER_BUTTON_START:       joypad->set_joypad_button(Joypad::BUTTON::START); break;
                case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:joypad->set_joypad_button(Joypad::BUTTON::SELECT); break;
                case SDL_CONTROLLER_BUTTON_DPAD_UP:     joypad->set_joypad_button(Joypad::BUTTON::UP); break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:   joypad->set_joypad_button(Joypad::BUTTON::DOWN); break;
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:   joypad->set_joypad_button(Joypad::BUTTON::LEFT); break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:  joypad->set_joypad_button(Joypad::BUTTON::RIGHT); break;
                case SDL_CONTROLLER_BUTTON_BACK:
                {
                    //SDL_AndroidBackButton();
                    backIsPressed = true;
                    //quit = true;
                    break;
                }
            }
            break;
        }

        case SDL_JOYBUTTONUP:
        {
            SDL_Log("JOYBUTTON UP %d", event.jbutton.button);

            switch (event.jbutton.button)
            {
                case SDL_CONTROLLER_BUTTON_A:
                case SDL_CONTROLLER_BUTTON_Y:           joypad->release_joypad_button(Joypad::BUTTON::A); break;
                case SDL_CONTROLLER_BUTTON_B:
                case SDL_CONTROLLER_BUTTON_X:           joypad->release_joypad_button(Joypad::BUTTON::B); break;
                case SDL_CONTROLLER_BUTTON_START:       joypad->release_joypad_button(Joypad::BUTTON::START); break;
                case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:joypad->release_joypad_button(Joypad::BUTTON::SELECT); break;
                case SDL_CONTROLLER_BUTTON_DPAD_UP:     joypad->release_joypad_button(Joypad::BUTTON::UP); break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:   joypad->release_joypad_button(Joypad::BUTTON::DOWN); break;
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:   joypad->release_joypad_button(Joypad::BUTTON::LEFT); break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:  joypad->release_joypad_button(Joypad::BUTTON::RIGHT); break;
                case SDL_CONTROLLER_BUTTON_BACK:
                {
                    if (backIsPressed)
                    {
                        backIsPressed = false;
                        backPressed++;
                        SDL_Log("Back Pressed: %d", backPressed);
                    }
                }
            }
            break;
        }

        case SDL_FINGERDOWN:
        {
            if (is_android_tv)
            {
                break;
            }

            if (fingerTouchMap.count(event.tfinger.fingerId) == 0)
            {   // New finger added to map,
                // initialize to nullptr
                fingerTouchMap[event.tfinger.fingerId] = nullptr;
            }

            x = event.tfinger.x * drawableWidth;
            y = event.tfinger.y * drawableHeight;
            //SDL_Log("SDL_FINGERDOWN: [%d, %d]", x, y);

            // Set joypad buttons if setup
            if (fingerTouchMap[event.tfinger.fingerId] == nullptr)
            {
                for (SDLTextureRect* textureButton : buttonTextures)
                {
                    if (textureButton->joypad_button != Joypad::BUTTON::NONE
                        && intersects(x, y, *textureButton))
                    {
                        if (!move_buttons && !resize_buttons)
                        {   // Tell emulator button is pressed
                            emu->set_joypad_button(textureButton->joypad_button);
                        }
                        // Map finger as currently being used
                        fingerTouchMap[event.tfinger.fingerId] = textureButton;
                    }
                }
            }

            // DPAD
            if (fingerTouchMap[event.tfinger.fingerId] == nullptr &&
                intersects(x, y, dpad.bounding_rect))
            {
                // Map finger as currently being used
                //SDL_Log("SDL_FINGERMOTION DPAD");
                pressedDPAD = true;
                fingerTouchMap[event.tfinger.fingerId] = &texture_button_dpad;

                // Animate circle DPAD if not moving buttons or not resizing buttons
                if (!move_buttons && !resize_buttons)
                {
                    // Update circular dpad's position
                    texture_button_dpad.rect = dpad.move(x, y, texture_button_dpad.rect);
                    int dpadDirection = dpad.getDirection(texture_button_dpad.rect);

                    if (dpadDirection & (uint8_t)GAMEPAD_BUTTON::UP)
                    {
                        emu->set_joypad_button(Joypad::BUTTON::UP);
                    }
                    else if (dpadDirection & (uint8_t)GAMEPAD_BUTTON::DOWN)
                    {
                        emu->set_joypad_button(Joypad::BUTTON::DOWN);
                    }

                    if (dpadDirection & (uint8_t)GAMEPAD_BUTTON::LEFT)
                    {
                        emu->set_joypad_button(Joypad::BUTTON::LEFT);
                    }
                    else if (dpadDirection & (uint8_t)GAMEPAD_BUTTON::RIGHT)
                    {
                        emu->set_joypad_button(Joypad::BUTTON::RIGHT);
                    }
                }
            }

            // BG Color change
            if (fingerTouchMap[event.tfinger.fingerId] == nullptr &&
                intersects(x, y, texture_button_bgcolor_change.rect))
            {
                int next_bg_color = bg_color + 1;
                if (next_bg_color == BGColor::LAST_COLOR)
                {
                    next_bg_color = BGColor::BLACK;
                }
                setBGColor(static_cast<BGColor>(next_bg_color));
            }

            // DMG Palette change
            if (fingerTouchMap[event.tfinger.fingerId] == nullptr &&
                intersects(x, y, texture_button_dmgpalette_change.rect))
            {
                if (emu)
                {
                    emu->changeCGBPalette();
                }
            }

            break;
        }

        case SDL_FINGERMOTION:
        {
            if (is_android_tv)
            {
                break;
            }

            x = event.tfinger.x * drawableWidth;
            y = event.tfinger.y * drawableHeight;
            //SDL_Log("SDL_FINGERDOWN: [%d, %d]", x, y);

            // Move buttons if moving buttons
            if (move_buttons &&
                fingerTouchMap[event.tfinger.fingerId] != nullptr)
            {   // Move button texture
                SDLTextureRect* buttonTexture = fingerTouchMap[event.tfinger.fingerId];

                if (buttonTexture->joypad_button != Joypad::BUTTON::NONE
                    && intersects(x, y, *buttonTexture))
                {
                    buttonTexture->rect.x = x - (buttonTexture->rect.w / 2);
                    buttonTexture->rect.y = y - (buttonTexture->rect.h / 2);
                }
            }

            // DPAD
            if (pressedDPAD
                && fingerTouchMap[event.tfinger.fingerId] == &texture_button_dpad)
            {
                //SDL_Log("SDL_FINGERMOTION DPAD");

                if (!move_buttons && !resize_buttons) {
                    // Update circular dpad's position
                    texture_button_dpad.rect = dpad.move(x, y, texture_button_dpad.rect);
                    const uint8_t dpadDirection = dpad.getDirection(texture_button_dpad.rect);

                    if (dpadDirection & (uint8_t)GAMEPAD_BUTTON::UP) {   // Set up
                        SDL_Log("Setting DPAD UP");
                        emu->release_joypad_button(Joypad::BUTTON::DOWN);
                        emu->set_joypad_button(Joypad::BUTTON::UP);
                    } else if (dpadDirection & (uint8_t)GAMEPAD_BUTTON::DOWN) {   // Set down
                        SDL_Log("Setting DPAD DOWN");
                        emu->release_joypad_button(Joypad::BUTTON::UP);
                        emu->set_joypad_button(Joypad::BUTTON::DOWN);
                    } else {   // Clear up down
                        SDL_Log("Clearing DPAD UP and DOWN");
                        emu->release_joypad_button(Joypad::BUTTON::UP);
                        emu->release_joypad_button(Joypad::BUTTON::DOWN);
                    }

                    if (dpadDirection & (uint8_t)GAMEPAD_BUTTON::LEFT) {   // Set left
                        SDL_Log("Setting DPAD LEFT");
                        emu->release_joypad_button(Joypad::BUTTON::RIGHT);
                        emu->set_joypad_button(Joypad::BUTTON::LEFT);
                    } else if (dpadDirection & (uint8_t)GAMEPAD_BUTTON::RIGHT) {   // Set right
                        SDL_Log("Setting DPAD RIGHT");
                        emu->release_joypad_button(Joypad::BUTTON::LEFT);
                        emu->set_joypad_button(Joypad::BUTTON::RIGHT);
                    } else {   // Clear left right
                        SDL_Log("Resetting DPAD LEFT and RIGHT");
                        emu->release_joypad_button(Joypad::BUTTON::LEFT);
                        emu->release_joypad_button(Joypad::BUTTON::RIGHT);
                    }
                }
                else
                {
                    if (move_buttons)
                    {   // Move both DPAD textures
                        texture_button_dpad.rect.x = x - (texture_button_dpad.rect.w / 2);
                        texture_button_dpad.rect.y = y - (texture_button_dpad.rect.h / 2);

                        //texture_button_dpad_border.rect.x = x - (texture_button_dpad_border.rect.w / 2);
                        //texture_button_dpad_border.rect.y = y - (texture_button_dpad_border.rect.h / 2);

                        dpad.initialize(texture_button_dpad.rect);
                        texture_button_dpad_border.rect = dpad.bounding_rect;
                    }
                }
            }

            break;
        }

        case SDL_FINGERUP:
        {
            if (is_android_tv)
            {
                break;
            }

            x = event.tfinger.x * drawableWidth;
            y = event.tfinger.y * drawableHeight;
            SDL_Log("SDL_FINGERUP: %d, [%d, %d]",
                event.tfinger.fingerId, x, y);

            // Set joypad buttons if setup
            if (fingerTouchMap[event.tfinger.fingerId] != nullptr)
            {
                for (SDLTextureRect *textureButton : buttonTextures)
                {
                    if (textureButton->joypad_button != Joypad::BUTTON::NONE
                        && fingerTouchMap[event.tfinger.fingerId] == textureButton)
                    {
                        joypad->release_joypad_button(textureButton->joypad_button);
                        fingerTouchMap[event.tfinger.fingerId] = nullptr;
                    }
                }
            }

            // DPAD
            if (fingerTouchMap[event.tfinger.fingerId] == &texture_button_dpad)
            {
                SDL_Log("Resetting DPAD positionand releasing DPAD presses");
                pressedDPAD = false;
                fingerTouchMap[event.tfinger.fingerId] = nullptr;

                // Reset DPAD direction
                texture_button_dpad.rect = dpad.orig_rect;
                joypad->release_joypad_button(Joypad::BUTTON::UP);
                joypad->release_joypad_button(Joypad::BUTTON::DOWN);
                joypad->release_joypad_button(Joypad::BUTTON::LEFT);
                joypad->release_joypad_button(Joypad::BUTTON::RIGHT);
            }
            break;
        }

        case SDL_MULTIGESTURE:
        {
            if (is_android_tv)
            {
                break;
            }

            // Only allow pinching if resizing buttons is enabled
            if (!resize_buttons || event.mgesture.numFingers != 2)
            {
                break;
            }

            // Detect pinch
            if (std::fabs(event.mgesture.dDist) > 0.002)
            {   // Pinching found
                x = event.mgesture.x * drawableWidth;
                y = event.mgesture.y * drawableHeight;

                // Figure out which button texture is being pinched
                for (SDLTextureRect* textureButton : buttonTextures)
                {
                    if (textureButton->joypad_button != Joypad::BUTTON::NONE
                        && intersects(x, y, *textureButton))
                    {
                        textureButton->rect.h *= 1.0f + (event.mgesture.dDist * 2);
                        textureButton->rect.w *= 1.0f + (event.mgesture.dDist * 2);
                    }
                }

                // Resize DPAD
                if (intersects(x, y, texture_button_dpad_border))
                {
                    texture_button_dpad.rect.h *= 1.0f + (event.mgesture.dDist * 2);
                    texture_button_dpad.rect.w *= 1.0f + (event.mgesture.dDist * 2);

                    dpad.initialize(texture_button_dpad.rect);
                    texture_button_dpad_border.rect = dpad.bounding_rect;
                }

            }
            break;
        }

        case SDL_WINDOWEVENT:
        {
            switch (event.window.event)
            {
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    screen_orientation_changed = true;

                    // Update window size
                    SDL_GL_GetDrawableSize(window, &drawableWidth, &drawableHeight);
                    break;
                case SDL_WINDOWEVENT_MINIMIZED:
                    pause();
                    break;
                case SDL_WINDOWEVENT_RESTORED:
                    resume();
                    break;
            }
        }

    } // switch(event.type)

} // end handleSDLEvent()

void SDLWindowAndroid::update(bool& run)
{
    if (quit)
    {
        run = false;
        return;
    }

    if (!initialized_SDL)
    {
        SDL_Log("Returning update() as SDL is not initialized");
        return;
    }

    SDL_Event event;
    SDL_PollEvent(&event);
    numEventsPerFrame++;

    bool sameEventType = event.type == prevEvent.type;
    bool sameEventKey = event.key.keysym.sym == prevEvent.key.keysym.sym;
    bool sameEventFinger = event.tfinger.x == prevEvent.tfinger.x
                           && event.tfinger.y == prevEvent.tfinger.y;

    //if (event.key.repeat == 0)
    {
        handleSDLEvent(event, run);
    }

    if (quit)
    {
        SDL_Log("Quitting SDL");
        run = false;
        return;
    }

    if (!sdl_joystick &&
        SDL_NumJoysticks() > 0 &&
        SDL_NumJoysticks() > num_joypads_looked_at)
    {
        SDL_Log("Found %d joysticks, opening joystick 0", SDL_NumJoysticks());
        num_joypads_looked_at = 0;

        int pickController = -1;
        for (int i = 0; i < SDL_NumJoysticks(); i++)
        {
            sdl_joystick = SDL_JoystickOpen(i);
            SDL_Log("Joystick %d,"
                    "name: %s, number of axes: %d, number of buttons %d",
                    i,
                    SDL_JoystickNameForIndex(i),
                    SDL_JoystickNumAxes(sdl_joystick),
                    SDL_JoystickNumButtons(sdl_joystick));

            num_joypads_looked_at++;

            // Jankily check if using NVIDIA Controller
            std::string joystickName = std::string(SDL_JoystickNameForIndex(i));
            if (joystickName.find("Controller") != std::string::npos)
            {
                pickController = i;
            }

            // Android Accelerometer <- Kirby tilt n tumble? B)

            SDL_JoystickClose(sdl_joystick);
            sdl_joystick = nullptr;
        }

        if (pickController != -1)
        {
            SDL_Log("Opening joystick %d", pickController);
            sdl_joystick = SDL_JoystickOpen(pickController);
        }

        if (sdl_joystick)
        {
            SDL_Log("Successfully initialized sdl_joystick,"
                    "name: %s, number of axes: %d, number of buttons %d",
                    SDL_JoystickNameForIndex(pickController),
                    SDL_JoystickNumAxes(sdl_joystick),
                    SDL_JoystickNumButtons(sdl_joystick));
        }
        else
        {
            SDL_Log("Failed to open joystick %d", pickController);
        }
    }

    if (joypadx)
    {
        //joypadx->refreshButtonStates(joypadx->findControllers());
    }

    if (new_frame)
    {
        new_frame = false;
        //SDL_Log("Number of events per frame: %d", numEventsPerFrame);
        numEventsPerFrame = 0;
        presentFrame(buttonTextures);
        fps++;
    }

    // Calculate FPS
    const auto currTime = std::chrono::duration<double>(
            std::chrono::system_clock::now().time_since_epoch());
    const std::chrono::milliseconds currTimeMilli =
            std::chrono::duration_cast<std::chrono::milliseconds>
                    (currTime - second_start_clock);

    if (currTimeMilli.count() >= 1000)
    {   // 1 second has elapsed
        SDL_Log("FPS: %d", fps);
        fps = 0;
        second_start_clock = std::chrono::duration<double>(
                std::chrono::system_clock::now().time_since_epoch());
    }

    prevEvent = event;
}

void SDLWindowAndroid::pause()
{
    stopEmulator();
}

void SDLWindowAndroid::resume()
{
    startEmulator();
}

void SDLWindowAndroid::stopEmulator()
{
    if (!emu)
    {
        return;
    }

    SDL_Log("Doing emu->stop()");
    emu->stop();
    SDL_Log("emu->stop() completed");

    if (emu_thread && emu_thread->joinable())
    {
        try
        {
            SDL_Log("Doing emu_thread.join()");
            emu_thread->join();
            SDL_Log("emu_thread.join() completed, resetting emu_thread");
            emu_thread.reset();
            SDL_Log("emu_thread.reset() completed");
        }
        catch (const std::exception& e)
        {
            SDL_Log("Caught exception while join()ing emu_thread, err: %s", e.what());
        }
    }
}

void SDLWindowAndroid::startEmulator()
{
    if (!emu)
    {
        logger->error("Refusing to start non existant emulator");
        return;
    }

    if (emu_thread && emu_thread->joinable())
    {
        SDL_Log("Doing emu_thread.join()");
        emu_thread->join();
        SDL_Log("emu_thread.join() completed, resetting emu_thread");
        emu_thread.reset();
        SDL_Log("emu_thread.reset() completed");
    }

    int ret = SDL_SetThreadPriority(SDL_ThreadPriority::SDL_THREAD_PRIORITY_TIME_CRITICAL);

    // Have emulator tick in its own thread
    logger->info("Starting emulator thread");
    SDL_Log("Starting emulator thread");
    emu_thread = std::unique_ptr<std::thread>(new std::thread([&]()
    {
        int ret = SDL_SetThreadPriority(SDL_ThreadPriority::SDL_THREAD_PRIORITY_TIME_CRITICAL);
        try {
          emu->run();
        }
        catch (...) {
          SDL_Log("Caught exception in SDLWindowAndroid::emu_thread");
        }
    }));

    second_start_clock = std::chrono::duration<double>(
            std::chrono::system_clock::now().time_since_epoch());
}

std::string SDLWindowAndroid::getFileExtension(const std::string& filepath)
{
    std::string ret = "";
    size_t index = filepath.find_last_of(".");
    if (index != std::string::npos)
    {   // Found at least one "."
        ret = filepath.substr(index + 1);
    }

    return ret;
}

bool SDLWindowAndroid::romIsValid(const std::string& filepath)
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

void SDLWindowAndroid::takeSaveState()
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

void SDLWindowAndroid::loadSaveState()
{
    if (!emu_savestate || !emu)
    {   // Need a save state and an emulator to load a savestate
        return;
    }

    emu->stop();
    if (emu_thread && emu_thread->joinable())
    {
        // Close emulator thread
        SDL_Log("Doing emu_thread.join()");
        emu_thread->join();
        SDL_Log("emu_thread.join() completed, resetting emu_thread");
        emu_thread.reset();
        SDL_Log("emu_thread.reset() completed");
    }

    // Load emulator savestate into emulator
    *emu.get() = *emu_savestate.get();

    hookToEmulator(emu);
    startEmulator();
}

bool SDLWindowAndroid::loadImageIntoTextureRect(const std::string& img_path, SDLTextureRect& texture_rect)
{
    // Load .png into SDLTexture
    texture_rect.texture = IMG_LoadTexture(renderer, img_path.c_str());
    if (!texture_rect.texture)
    {
        SDL_Log("Failed to load image: %s, into texture, error %s",
            img_path.c_str(),
            IMG_GetError());
        return false;
    }

    // Resize SDLTextureRect
    resetTextureSize(texture_rect);

    // Name SDLTextureRect
    texture_rect.name = img_path.substr(0, img_path.find("."));

    return true;
}

void SDLWindowAndroid::resetTextureSize(SDLTextureRect& texture_rect)
{
    SDL_QueryTexture(texture_rect.texture,
            NULL,
            NULL,
            &texture_rect.rect.w,
            &texture_rect.rect.h);
}

// Returns true if point intersects drawn texture,
// false otherwise
bool SDLWindowAndroid::intersects(const int x, const int y, const SDLTextureRect& texture_rect) const
{
    const SDL_Rect& rect = texture_rect.rect;
    const bool intersectsX =  (x >= rect.x) && (x <= rect.x + rect.w);
    const bool intersectsY = (y >= rect.y) && (y <= rect.y + rect.h);

    return intersectsX && intersectsY;
}


// Returns true if point intersects drawn texture,
// false otherwise
bool SDLWindowAndroid::intersects(const int x, const int y, const SDL_Rect& rect) const
{
    const bool intersectsX =  (x >= rect.x) && (x <= rect.x + rect.w);
    const bool intersectsY = (y >= rect.y) && (y <= rect.y + rect.h);

    return intersectsX && intersectsY;
}

void SDLWindowAndroid::presentFrame(const std::vector<SDLTextureRect*>& button_textures)
{
    if (screen_orientation_changed)
    {   // Clear screen double buffer so frame is
        // drawn with black borders
        screen_orientation_changed = false;
        clearScreenDoubleBuffer();
    }

    std::lock_guard<std::mutex> lg(renderer_mutex);
    //SDL_Log("SDL_UpdateTexture()");

    SDL_RenderClear(renderer);

    if (move_buttons || resize_buttons)
    {   // Draw grid
        drawGrid(0x00, 0xFF, 0xFF, SDL_ALPHA_OPAQUE);                // Aqua Blue
        drawLinesThroughButtons(0xFF, 0xFF, 0x00, SDL_ALPHA_OPAQUE); // Yellow
    }

    // Copy frame from emulator into SDL texture
    SDL_UpdateTexture(screen_texture_rect.texture, NULL, curr_frame.data(), SCREEN_PIXEL_W * sizeof(SDL_Color));
    SDL_RenderCopy(renderer,
               screen_texture_rect.texture,
               NULL,
               &screen_texture_rect.rect);

    if (!is_android_tv)
    {   // Draw buttons
        for (SDLTextureRect *textureRect : button_textures)
        {
            // Don't draw DMG palette toggle button if
            // game loaded is .gbc
            if (textureRect == &texture_button_dmgpalette_change &&
                emu && emu->isColorGB())
            {
                continue;
            }
            SDL_RenderCopy(renderer,
                           textureRect->texture,
                           NULL,
                           &textureRect->rect);
        }
    }

    // Present frame
    SDL_RenderPresent(renderer);
}

void SDLWindowAndroid::setBGColor(const BGColor color)
{
    bg_color = color;
    if (renderer)
    {
        const RGB rgb = GetColor(color);
        SDL_SetRenderDrawColor(renderer, rgb.r, rgb.g, rgb.b, 255);
    }
}

void SDLWindowAndroid::clearScreenDoubleBuffer()
{
    std::lock_guard<std::mutex> lg(renderer_mutex);
    // Clear first frame in double buffer
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
    // Clear second frame in double buffer
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}

void SDLWindowAndroid::toggleMoveButtons(const bool val)
{
    move_buttons = val;
}

void SDLWindowAndroid::toggleResizeButtons(const bool val)
{
    resize_buttons = val;
}

void SDLWindowAndroid::drawGrid(const uint8_t _r, const uint8_t _g,
        const uint8_t _b, const uint8_t _a)
{
    // Save original Render draw color
    uint8_t r, g, b, a;
    r = g = b = a = 0;
    SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);

    // Set new draw color
    SDL_SetRenderDrawColor(renderer, _r, _g, _b, _a);

    // Draw grid
    const int pixel_line_width = screen_texture_rect.rect.x ? screen_texture_rect.rect.x : drawableWidth / 20;

    for (int x = 0; x < drawableWidth; x += pixel_line_width) {
        SDL_RenderDrawLine(renderer, x, 0, x, drawableHeight);
    }
    for (int y = 0; y < drawableHeight; y += pixel_line_width) {
        SDL_RenderDrawLine(renderer, 0, y, drawableWidth, y);
    }

    // Set Render draw color back
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
}

void SDLWindowAndroid::drawLinesThroughButtons(const uint8_t _r, const uint8_t _g,
        const uint8_t _b, const uint8_t _a)
{
    // Save original Render draw color
    uint8_t r, g, b, a;
    r = g = b = a = 0;
    SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);

    // Set new draw color
    SDL_SetRenderDrawColor(renderer, _r, _g, _b, _a);

    // Draw lines through touched buttons
    for (auto pair : fingerTouchMap)
    {
        if (pair.second != nullptr)
        {   // Button is being touched
            const SDL_Rect& rect = pair.second->rect;

            // Get width of button
            const int width_x = rect.w / 2;
            const int width_y = rect.h / 2;

            // Calculate where to draw lines
            const int middle_x = rect.x + width_x;
            const int middle_y = rect.y + width_y;

            // Draw two lines through button
            SDL_RenderDrawLine(renderer, middle_x, 0, middle_x, drawableHeight);    // Y = 0
            SDL_RenderDrawLine(renderer, 0, middle_y, drawableWidth, middle_y);     // X = 0
        }
    }

    // Set Render draw color back
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
}

void SDLWindowAndroid::saveSDLTextureRects()
{
    std::ofstream out_file;
    out_file.open(config_path, std::ios::binary);

    if (!out_file.is_open())
    {
        return;
    }

    for (SDLTextureRect* texture_rect : buttonTextures)
    {
        std::string out_line =
                texture_rect->name + "/"
                + std::to_string(texture_rect->rect.x) + "/"
                + std::to_string(texture_rect->rect.y) + "/"
                + std::to_string(texture_rect->rect.w) + "/"
                + std::to_string(texture_rect->rect.h) + "/";

        out_file << out_line << std::endl;
    }

    out_file.close();
}

void SDLWindowAndroid::loadSDLTextureRects()
{
    std::ifstream in_file;
    in_file.open(config_path, std::ios::binary);

    if (!in_file.is_open())
    {
        return;
    }

    for (std::string in_line; std::getline(in_file, in_line);)
    {
        const std::vector<std::string> split_str = splitString(in_line, "/");

        if (split_str.size() != 5)
        {
            continue;
        }

        // Get SDLTextureRect with same name
        for (SDLTextureRect* texture_rect : buttonTextures)
        {
            if (texture_rect &&
                texture_rect->name == split_str.at(0))
            {   // Load texture pos and size
                texture_rect->rect.x = std::stoi(split_str.at(1));
                texture_rect->rect.y = std::stoi(split_str.at(2));
                texture_rect->rect.w = std::stoi(split_str.at(3));
                texture_rect->rect.h = std::stoi(split_str.at(4));
                break;
            }
        }
    }

    // Load DPAD bounding circle
    dpad.initialize(texture_button_dpad.rect);
    texture_button_dpad_border.rect = dpad.bounding_rect;
}

std::vector<std::string> SDLWindowAndroid::splitString(std::string in, const std::string& delimiter)
{
    std::vector<std::string> ret;

    while (in.size())
    {
        const int index = in.find(delimiter);
        if (index != std::string::npos)
        {
            ret.push_back(in.substr(0, index));
            in = in.substr(index + delimiter.size());
        }
        else
        {
            ret.push_back(in);
            in = "";
        }
    }

    return ret;
}

void SDLWindowAndroid::changeCGBPalette()
{
    if (emu)
    {
        emu->changeCGBPalette();
    }
}