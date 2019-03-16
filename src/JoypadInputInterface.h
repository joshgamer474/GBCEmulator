#ifndef JOYPAD_INPUT_INTERFACE_H
#define JOYPAD_INPUT_INTERFACE_H

#include <memory>
#include <unordered_map>
#include <vector>

class Joypad;

class JoypadInputInterface
{
public:
    JoypadInputInterface() {}
    JoypadInputInterface(std::shared_ptr<Joypad>) {}
    //virtual JoypadInputInterface() = 0;
    //virtual JoypadInputInterface(std::shared_ptr<Joypad> _joypad) = 0;
    virtual ~JoypadInputInterface() {}

    virtual void setJoypad(std::shared_ptr<Joypad> _joypad) = 0;
    virtual void refreshButtonStates(const int & controller) = 0;
    virtual int findControllers() = 0;

protected:
    //void init();
    virtual std::unordered_map<int, bool> initButtonStatesMap() const = 0;
    virtual int getJoypadButtonFromMask(const int & mask) const = 0;
    virtual bool isConnected(int controller) const = 0;

    //std::unordered_map<int, bool> prev_button_states;
    //std::shared_ptr<Joypad> joypad;
};

#endif // JOYPAD_INPUT_INTERFACE_H