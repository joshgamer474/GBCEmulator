#include <JoypadGeneric.h>
#include <Joypad.h>

#include <SDL3/SDL_gamepad.h>

JoypadGeneric::JoypadGeneric()
{
  init();
}

JoypadGeneric::JoypadGeneric(std::shared_ptr<Joypad> _joypad)
  : joypad(_joypad)
{
  init();
}

JoypadGeneric::~JoypadGeneric()
{

}

void JoypadGeneric::setJoypad(std::shared_ptr<Joypad> _joypad)
{
  joypad = _joypad;
}

void JoypadGeneric::init()
{
  prev_button_states = initButtonStatesMap();
}

int JoypadGeneric::findControllers()
{
  const bool ret = SDL_HasGamepad();
  return ret;
  int numControllersConnected = -1;

  SDL_JoystickID *ids = SDL_GetGamepads(&numControllersConnected);
  do
  {
    if (ids[numControllersConnected] != 0)
    {
      return ids[numControllersConnected];
      //numControllersConnected++;
    }
    else
    {
      break;
    }
  }
  while (true);
  /*SDL_Gamepad* gamepad = NULL;

  for (int i = 0; i < numControllersConnected; i++)
  {
    SDL_Gamepad* gp = SDL_OpenGamepad(ids[i]);
    if (gp == nullptr)
    {
      gamepad = gp;
    }

    if (i > 0)
    {
      SDL_CloseGamepad(gp);
    }
  }

  if (!gamepad)
  {
  }*/
  return numControllersConnected;
}

void JoypadGeneric::refreshButtonStates(const int & controller)
{
  if (controller < 0 ||
    controller > 4)
  {
    return;
  }

  if (!isConnected(controller))
  {   // Controller not connected!
    return;
  }

  // Get gamepad
  SDL_Gamepad* gamepad = SDL_GetGamepadFromID(static_cast<SDL_JoystickID>(controller));

  for (auto& pair : prev_button_states)
  {
    const bool currState = SDL_GetGamepadButton(gamepad, static_cast<SDL_GamepadButton>(pair.first));
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
}

std::unordered_map<int, bool> JoypadGeneric::initButtonStatesMap() const
{
  return std::unordered_map<int, bool>
  {
    { SDL_GAMEPAD_BUTTON_SOUTH,         false },
    { SDL_GAMEPAD_BUTTON_EAST,          false },
    { SDL_GAMEPAD_BUTTON_WEST,          false },
    { SDL_GAMEPAD_BUTTON_NORTH,         false },
    { SDL_GAMEPAD_BUTTON_DPAD_LEFT,     false },
    { SDL_GAMEPAD_BUTTON_DPAD_RIGHT,    false },
    { SDL_GAMEPAD_BUTTON_DPAD_UP,       false },
    { SDL_GAMEPAD_BUTTON_DPAD_DOWN,     false },
    { SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, false },
    { SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,false },
    { SDL_GAMEPAD_BUTTON_LEFT_STICK,    false },
    { SDL_GAMEPAD_BUTTON_RIGHT_STICK,   false },
    { SDL_GAMEPAD_BUTTON_BACK,          false },
    { SDL_GAMEPAD_BUTTON_START,         false },
  };
}

int JoypadGeneric::getJoypadButtonFromMask(const int & mask) const
{
  switch (mask)
  {
    case SDL_GAMEPAD_BUTTON_SOUTH:             return Joypad::BUTTON::A;
    case SDL_GAMEPAD_BUTTON_EAST:             return Joypad::BUTTON::B;
    case SDL_GAMEPAD_BUTTON_WEST:             return Joypad::BUTTON::B;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT:     return Joypad::BUTTON::LEFT;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:    return Joypad::BUTTON::RIGHT;
    case SDL_GAMEPAD_BUTTON_DPAD_UP:       return Joypad::BUTTON::UP;
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN:     return Joypad::BUTTON::DOWN;
    case SDL_GAMEPAD_BUTTON_BACK:          return Joypad::BUTTON::SELECT;
    case SDL_GAMEPAD_BUTTON_START:         return Joypad::BUTTON::START;
    default:
      return -1;
  }
}

bool JoypadGeneric::isConnected(int controller) const
{
  SDL_Gamepad* gp = SDL_OpenGamepad(controller);
  return SDL_GamepadConnected(gp);
}