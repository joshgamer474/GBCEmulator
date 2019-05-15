#ifndef SRC_DEBUGGER_H
#define SRC_DEBUGGER_H

#include <imgui.h>
#include <GL/glew.h>
#include <GBCEmulator.h>
#include <memory>

extern "C" {
#include <SDL.h>
}

#define IMGUI_IMPL_OPENGL_LOADER_GLEW 1

class Debugger
{
public:
    Debugger(SDL_GLContext* glContext, SDL_Window* window);
    virtual ~Debugger();

    void draw(SDL_Event* event, SDL_Renderer* renderer, SDL_Texture* screen_texture);
    void setEmulator(std::shared_ptr<GBCEmulator> emu);

private:
    std::shared_ptr<GBCEmulator> emu;
    SDL_GLContext* glContext;
    SDL_Window* window;
    bool draw_vram_debugger;
    bool draw_audio_debugger;
    bool draw_register_debugger;
};

#endif // SRC_DEBUGGER_H