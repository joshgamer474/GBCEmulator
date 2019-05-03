#ifndef SCREEN_INTERFACE_Hj
#define SCREEN_INTERFACE_H

#include <GBCEmulator.h>
#include <array>
#include <memory>
extern "C" {
#include <SDL.h>
}

class ScreenInterface
{
public:
    virtual ~ScreenInterface() {}

    virtual void hookToEmulator(std::shared_ptr<GBCEmulator> emulator) = 0;
    virtual void display(std::array<SDL_Color, SCREEN_PIXEL_TOTAL> /* frame */) = 0;
};

#endif // SCREEN_INTERFACE_H