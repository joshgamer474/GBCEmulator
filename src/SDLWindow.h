#ifndef SDL_WINDOW_H
#define SDL_WINDOW_H

#include <ScreenInterface.h>
#include <GBCEmulator.h>
#include <memory>

extern "C" {
#include <SDL.h>
}

class SDLWindow : public ScreenInterface
{
public:
    SDLWindow();
    virtual ~SDLWindow();

    void setEmulator(std::shared_ptr<GBCEmulator> emulator);
    void display(SDL_Color * frame);

private:
    void init();
    void updateWindowTitle(const uint64_t & framerate);

    std::shared_ptr<GBCEmulator> emu;
    SDL_GLContext glContext;
    SDL_Window* window;
    SDL_Surface* screenSurface;
    SDL_Renderer* renderer;
    SDL_Texture* screen_texture;
    SDL_Rect screen_texture_rect;
    uint64_t framerate;
};

#endif // SDL_WINDOW_H