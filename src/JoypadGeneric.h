#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include <JoypadInputInterface.h>

class JoypadGeneric : public JoypadInputInterface
{
public:
    JoypadGeneric();
    JoypadGeneric(std::shared_ptr<Joypad> _joypad);
    virtual ~JoypadGeneric();

    void setJoypad(std::shared_ptr<Joypad> _joypad);
    void refreshButtonStates(const int & controller);
    int findControllers();

private:
    void init();
    std::unordered_map<int, bool> initButtonStatesMap() const;
    int getJoypadButtonFromMask(const int & mask) const;
    bool isConnected(int controller) const;

    std::unordered_map<int, bool> prev_button_states;
    std::shared_ptr<Joypad> joypad;
};