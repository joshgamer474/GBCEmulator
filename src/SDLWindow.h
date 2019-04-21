#ifndef SDL_WINDOW_H
#define SDL_WINDOW_H

#include <ScreenInterface.h>
#include <GBCEmulator.h>
#include <JoypadXInput.h>
#include <memory>
#include <string>
#include <thread>

extern "C" {
#include <SDL.h>
}

class SDLWindow : public ScreenInterface
{
public:
    SDLWindow();
    virtual ~SDLWindow();

    void display(SDL_Color * frame);
    void hookToEmulator(std::shared_ptr<GBCEmulator> emulator);
    static bool romIsValid(const std::string & filepath);
    static std::string getFileExtension(const std::string & filepath);
    int run();

private:
    void init();
    void startEmulator();
    void updateWindowTitle(const uint64_t & framerate);

    std::shared_ptr<GBCEmulator> emu;
    std::shared_ptr<Joypad> joypad;
    std::shared_ptr<JoypadXInput> joypadx;
    std::thread emu_thread;
    SDL_GLContext glContext;
    SDL_Window* window;
    SDL_Surface* screenSurface;
    SDL_Renderer* renderer;
    SDL_Texture* screen_texture;
    SDL_Rect screen_texture_rect;
    uint64_t framerate;
};

#endif // SDL_WINDOW_H