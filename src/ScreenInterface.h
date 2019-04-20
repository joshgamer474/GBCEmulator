#ifndef SCREEN_INTERFACE_Hj
#define SCREEN_INTERFACE_H

#include <GBCEmulator.h>
#include <memory>
extern "C" {
#include <SDL.h>
}

class ScreenInterface
{
public:
    virtual ~ScreenInterface() {}

    virtual void setEmulator(std::shared_ptr<GBCEmulator> emulator) = 0;
    virtual void display(SDL_Color* frame) = 0;
};

#endif // SCREEN_INTERFACE_H