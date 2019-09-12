#ifndef SDL_WINDOW_H
#define SDL_WINDOW_H

#include <ScreenInterface.h>
#include <GBCEmulator.h>
#include <JoypadXInput.h>
#include <atomic>
#include <array>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <spdlog/spdlog.h>

extern "C" {
#include <SDL.h>
}

enum SDLRenderType : char {
    NEAREST_PIXEL = 0,
    LINEAR_FILTERING,
    ANISOTROPIC_FILTERING
};

class SDLWindow : public ScreenInterface
{
public:
    SDLWindow(const std::string& log_name = "SDLWindow.log");
    virtual ~SDLWindow();

    void display(std::array<SDL_Color, SCREEN_PIXEL_TOTAL> frame);
    void hookToEmulator(std::shared_ptr<GBCEmulator> emulator);
    static bool romIsValid(const std::string & filepath);
    static std::string getFileExtension(const std::string & filepath);
    int run(bool start_emu = false);

private:
    void init();
    void startEmulator();
    void updateWindowTitle(const std::string & framerate);
    void takeSaveState();
    void loadSaveState();

    std::shared_ptr<GBCEmulator> emu;
    std::shared_ptr<GBCEmulator> emu_savestate;
    std::shared_ptr<Joypad> joypad;
    std::shared_ptr<JoypadXInput> joypadx;
    std::shared_ptr<spdlog::logger> logger;
    std::array<SDL_Color, SCREEN_PIXEL_TOTAL> curr_frame;
    std::thread emu_thread;
    std::mutex renderer_mutex;
    SDL_GLContext glContext;
    SDL_Window* window;
    SDL_Surface* screenSurface;
    SDL_Renderer* renderer;
    SDL_Texture* screen_texture;
    SDL_Rect screen_texture_rect;
    uint64_t framerate;
    bool keep_aspect_ratio;
    std::atomic_bool have_new_frame;
};

#endif // SDL_WINDOW_H