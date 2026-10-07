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
  if (gamepad)
  {
    SDL_CloseGamepad(gamepad);
  }
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
  if (gamepad && SDL_GamepadConnected(gamepad))
  {
    return SDL_GetGamepadID(gamepad);
  }
  if (gamepad)
  {
    SDL_CloseGamepad(gamepad);
  }
  gamepad = nullptr;
  int count = 0;
  auto ids = SDL_GetGamepads(&count);
  for (int i = 0; i < count && !gamepad; ++i)
  {
    gamepad = SDL_OpenGamepad(ids[i]);
  }
  SDL_free(ids);
  return gamepad ? static_cast<int>(SDL_GetGamepadID(gamepad)) : -1;
}

uint32_t JoypadGeneric::mapButtons(uint32_t physical, int x, int y)
{
  uint32_t mask = 0;
  for (int i = 0; i < SDL_GAMEPAD_BUTTON_COUNT; ++i)
  {
    const auto button = mapButton(i);
    if (button >= 0 && (physical & (1u << i)))
    {
      mask |= 1u << button;
    }
  }
  if (x < -12000)
  {
    mask |= 1u << Joypad::LEFT;
  }
  if (x > 12000)
  {
    mask |= 1u << Joypad::RIGHT;
  }
  if (y < -12000)
  {
    mask |= 1u << Joypad::UP;
  }
  if (y > 12000)
  {
    mask |= 1u << Joypad::DOWN;
  }
  return mask;
}

uint32_t JoypadGeneric::pollButtons(SDL_Gamepad* pad)
{
  if (!pad || !SDL_GamepadConnected(pad))
  {
    return 0;
  }
  uint32_t physical = 0;
  for (int i = 0; i < SDL_GAMEPAD_BUTTON_COUNT; ++i)
  {
    if (SDL_GetGamepadButton(pad, static_cast<SDL_GamepadButton>(i)))
    {
      physical |= 1u << i;
    }
  }
  return mapButtons(physical, SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTX),
    SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTY));
}
void JoypadGeneric::refreshButtonStates(const int & controller)
{
  const auto current = controller >= 0 && gamepad &&
      SDL_GetGamepadID(gamepad) == static_cast<SDL_JoystickID>(controller)
      ? pollButtons(gamepad) : 0;
  const auto changed = current ^ applied_buttons;
  if (joypad)
  {
    for (int i = Joypad::DOWN; i <= Joypad::A; ++i)
    {
      if (!(changed & (1u << i)))
      {
        continue;
      }
      const auto button = static_cast<Joypad::BUTTON>(i);
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

int JoypadGeneric::mapButton(int mask)
{
  switch (mask)
  {
    case SDL_GAMEPAD_BUTTON_SOUTH:         return Joypad::BUTTON::A;
    case SDL_GAMEPAD_BUTTON_EAST:          return Joypad::BUTTON::B;
    case SDL_GAMEPAD_BUTTON_WEST:          return Joypad::BUTTON::B;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT:     return Joypad::BUTTON::LEFT;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:    return Joypad::BUTTON::RIGHT;
    case SDL_GAMEPAD_BUTTON_DPAD_UP:       return Joypad::BUTTON::UP;
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN:     return Joypad::BUTTON::DOWN;
    case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
    case SDL_GAMEPAD_BUTTON_BACK:          return Joypad::BUTTON::SELECT;
    case SDL_GAMEPAD_BUTTON_START:         return Joypad::BUTTON::START;
    default:
      return -1;
  }
}

int JoypadGeneric::getJoypadButtonFromMask(const int & mask) const
{
  return mapButton(mask);
}

bool JoypadGeneric::isConnected(int controller) const
{
  return gamepad && controller >= 0 &&
    SDL_GetGamepadID(gamepad) == static_cast<SDL_JoystickID>(controller) &&
    SDL_GamepadConnected(gamepad);
}
