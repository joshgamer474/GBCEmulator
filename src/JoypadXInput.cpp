#include <JoypadXInput.h>
#include <Joypad.h>

#ifdef _WIN32
#include <windows.h>
#include <xinput.h>
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

uint32_t JoypadXInput::pollButtons(int controller, bool& connected)
{
  connected = false;
  uint32_t mask = 0;
#ifdef _WIN32
  if (controller < 0 || controller >= XUSER_MAX_COUNT)
  {
    return 0;
  }

  XINPUT_STATE state{};
  if (XInputGetState(controller, &state) != ERROR_SUCCESS)
  {
    return 0;
  }
  connected = true;
  const XINPUT_GAMEPAD& pad = state.Gamepad;
  auto button = [&](Joypad::BUTTON bit, bool pressed)
  {
    if (pressed) mask |= 1u << bit;
  };
  button(Joypad::DOWN, (pad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) || pad.sThumbLY < -12000);
  button(Joypad::UP, (pad.wButtons & XINPUT_GAMEPAD_DPAD_UP) || pad.sThumbLY > 12000);
  button(Joypad::LEFT, (pad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) || pad.sThumbLX < -12000);
  button(Joypad::RIGHT, (pad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) || pad.sThumbLX > 12000);
  button(Joypad::START, pad.wButtons & XINPUT_GAMEPAD_START);
  button(Joypad::SELECT, pad.wButtons & (XINPUT_GAMEPAD_BACK | XINPUT_GAMEPAD_LEFT_SHOULDER));
  button(Joypad::B, pad.wButtons & (XINPUT_GAMEPAD_B | XINPUT_GAMEPAD_X));
  button(Joypad::A, pad.wButtons & XINPUT_GAMEPAD_A);
#endif
  return mask;
}

int JoypadXInput::findControllers()
{
    int numControllersConnected = -1;
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
  bool connected = false;
  const uint32_t current = pollButtons(controller, connected);
  const uint32_t changed = current ^ applied_buttons;
  if (joypad)
  {
    for (int i = Joypad::DOWN; i <= Joypad::A; ++i)
    {
      if (!(changed & (1u << i)))
      {
        continue;
      }

      const Joypad::BUTTON button = static_cast<Joypad::BUTTON>(i);
      if (current & (1u << i))
      {
        joypad->set_joypad_button(button);
      }
      else
      {
        joypad->release_joypad_button(button);
      }
    }
  }
  applied_buttons = current;
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

