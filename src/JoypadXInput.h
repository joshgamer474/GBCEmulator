#ifndef JOYPAD_XINPUT_H
#define JOYPAD_XINPUT_H

#include <memory>
#include <unordered_map>
#include <vector>

class Joypad;

class JoypadXInput
{
public:
    JoypadXInput();
    JoypadXInput(std::shared_ptr<Joypad> _joypad);
    virtual ~JoypadXInput();

    void setJoypad(std::shared_ptr<Joypad> _joypad);
    void refreshButtonStates(const int & controller);

private:
    void init();
    void findControllers();
    std::unordered_map<int, bool> initButtonStatesMap() const;
    int getJoypadButtonFromMask(const int & mask) const;

    std::vector<int> controllers;
    std::unordered_map<int, bool> prev_button_states;
    std::shared_ptr<Joypad> joypad;
};

#endif // JOYPAD_XINPUT_H