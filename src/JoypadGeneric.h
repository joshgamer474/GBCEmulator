#pragma once

#include <memory>
#include <cstdint>
#include <SDL3/SDL_gamepad.h>
#include <unordered_map>
#include <vector>

#include <JoypadInputInterface.h>

class JoypadGeneric : public JoypadInputInterface
{
public:
    JoypadGeneric();
    JoypadGeneric(std::shared_ptr<Joypad> _joypad);
    virtual ~JoypadGeneric();

    JoypadGeneric(const JoypadGeneric&) = delete;
    JoypadGeneric& operator=(const JoypadGeneric&) = delete;

    void setJoypad(std::shared_ptr<Joypad> _joypad);
    void refreshButtonStates(const int & controller);
    int findControllers();
    static int mapButton(int button);
    static uint32_t mapButtons(uint32_t physical, int x, int y);
    static uint32_t pollButtons(SDL_Gamepad* gamepad);

private:
    SDL_Gamepad* gamepad = nullptr;
    uint32_t applied_buttons = 0;
    void init();
    std::unordered_map<int, bool> initButtonStatesMap() const;
    int getJoypadButtonFromMask(const int & mask) const;
    bool isConnected(int controller) const;

    std::unordered_map<int, bool> prev_button_states;
    std::shared_ptr<Joypad> joypad;
};
