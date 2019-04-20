#include <SDLWindow.h>

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

void SDLWindow::setEmulator(std::shared_ptr<GBCEmulator> emulator)
{
    emu = emulator;
    emu->setFrameUpdateMethod(std::bind(&SDLWindow::display, this, std::placeholders::_1));
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