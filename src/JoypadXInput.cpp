#include <JoypadXInput.h>
#include <Joypad.h>

#ifdef _WIN32
#include <Windows.h>
#include <Xinput.h>
#endif // _WIN32

JoypadXInput::JoypadXInput()
{
    init();
}

JoypadXInput::JoypadXInput(std::shared_ptr<Joypad> _joypad)
    : joypad(_joypad)
{
    init();
}

JoypadXInput::~JoypadXInput()
{

}

void JoypadXInput::setJoypad(std::shared_ptr<Joypad> _joypad)
{
    joypad = _joypad;
}

void JoypadXInput::init()
{
#ifdef _WIN32
    prev_button_states = initButtonStatesMap();
#endif // _WIN32
}

int JoypadXInput::findControllers()
{
    int numControllersConnected = 0;
#ifdef _WIN32
    XINPUT_STATE state;
    ZeroMemory(&state, sizeof(XINPUT_STATE));

    for (int i = 0; i < XUSER_MAX_COUNT; i++)
    {
        if (XInputGetState(i, &state) == ERROR_SUCCESS)
        {
            numControllersConnected++;
        }
    }
#endif // _WIN32
    return numControllersConnected;
}

void JoypadXInput::refreshButtonStates(const int & controller)
{
#ifdef _WIN32
    if (controller < 0 ||
        controller > XUSER_MAX_COUNT)
    {
        return;
    }

    if (!isConnected(controller))
    {   // Controller not connected!
        return;
    }

    // Get gamepad state
    XINPUT_STATE controller_state;
    XInputGetState(controller, &controller_state);

    // Get gamepad button WORD
    const auto & buttons = controller_state.Gamepad.wButtons;

    for (auto & pair : prev_button_states)
    {   // int mask (first), bool state (second)
        const bool & currState = buttons & pair.first;
        if (currState != pair.second)
        {
            const int joypadButton = (getJoypadButtonFromMask(pair.first));
            if (joypad && joypadButton >= 0)
            {
                if (currState)
                {   // Button is now pressed
                    joypad->set_joypad_button(static_cast<Joypad::BUTTON>(joypadButton));
                }
                else
                {   // Button is now let go
                    joypad->release_joypad_button(static_cast<Joypad::BUTTON>(joypadButton));
                }
            }
            pair.second = currState;
        }
    }
#endif // _WIN32
}

std::unordered_map<int, bool> JoypadXInput::initButtonStatesMap() const
{
    return std::unordered_map<int, bool>
    {
#ifdef _WIN32
        { XINPUT_GAMEPAD_A,             false },
        { XINPUT_GAMEPAD_B,             false },
        { XINPUT_GAMEPAD_X,             false },
        { XINPUT_GAMEPAD_Y,             false },
        { XINPUT_GAMEPAD_DPAD_LEFT,     false },
        { XINPUT_GAMEPAD_DPAD_RIGHT,    false },
        { XINPUT_GAMEPAD_DPAD_UP,       false },
        { XINPUT_GAMEPAD_DPAD_DOWN,     false },
        { XINPUT_GAMEPAD_LEFT_SHOULDER, false },
        { XINPUT_GAMEPAD_RIGHT_SHOULDER,false },
        { XINPUT_GAMEPAD_LEFT_THUMB,    false },
        { XINPUT_GAMEPAD_RIGHT_THUMB,   false },
        { XINPUT_GAMEPAD_BACK,          false },
        { XINPUT_GAMEPAD_START,         false },
#endif // _WIN32
    };
}

int JoypadXInput::getJoypadButtonFromMask(const int & mask) const
{
    switch (mask)
    {
#ifdef _WIN32
        case XINPUT_GAMEPAD_A:             return Joypad::BUTTON::A;
        case XINPUT_GAMEPAD_B:             return Joypad::BUTTON::B;
        case XINPUT_GAMEPAD_X:             return Joypad::BUTTON::B;
        //case XINPUT_GAMEPAD_Y:             return Joypad::BUTTON::;
        case XINPUT_GAMEPAD_DPAD_LEFT:     return Joypad::BUTTON::LEFT;
        case XINPUT_GAMEPAD_DPAD_RIGHT:    return Joypad::BUTTON::RIGHT;
        case XINPUT_GAMEPAD_DPAD_UP:       return Joypad::BUTTON::UP;
        case XINPUT_GAMEPAD_DPAD_DOWN:     return Joypad::BUTTON::DOWN;
        //case XINPUT_GAMEPAD_LEFT_SHOULDER: return Joypad::BUTTON::;
        //case XINPUT_GAMEPAD_RIGHT_SHOULDER:return Joypad::BUTTON::;
        //case XINPUT_GAMEPAD_LEFT_THUMB:    return Joypad::BUTTON::;
        //case XINPUT_GAMEPAD_RIGHT_THUMB:   return Joypad::BUTTON::;
        case XINPUT_GAMEPAD_BACK:          return Joypad::BUTTON::SELECT;
        case XINPUT_GAMEPAD_START:         return Joypad::BUTTON::START;
#endif // _WIN32
        default:
            return -1;
    }
}

bool JoypadXInput::isConnected(int controller) const
{
#ifdef _WIN32
    XINPUT_STATE controller_state;
    return XInputGetState(controller, &controller_state) == ERROR_SUCCESS;
#else
    return false;
#endif // _WIN32
}