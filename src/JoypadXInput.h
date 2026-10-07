#pragma once

#include <JoypadInputInterface.h>
#include <memory>
#include <cstdint>
#include <unordered_map>
#include <vector>

//class Joypad;

class JoypadXInput : public JoypadInputInterface
{
public:
    JoypadXInput();
    JoypadXInput(std::shared_ptr<Joypad> _joypad);
    virtual ~JoypadXInput();

    void setJoypad(std::shared_ptr<Joypad> _joypad);
    void refreshButtonStates(const int & controller);
    int findControllers();

    // Snapshot bits use Joypad::BUTTON; does not mutate the emulator.
    static uint32_t pollButtons(int controller, bool& connected);

private:
    void init();
    std::unordered_map<int, bool> initButtonStatesMap() const;
    int getJoypadButtonFromMask(const int & mask) const;
    bool isConnected(int controller) const;

    std::unordered_map<int, bool> prev_button_states;
    uint32_t applied_buttons = 0;
    std::shared_ptr<Joypad> joypad;
};
