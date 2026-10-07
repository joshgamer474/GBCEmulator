#include "controller_input.h"

#include <JoypadGeneric.h>
#include <stdexcept>

void ControllerInput::State::snapshot()
{
  buttons = 0;
  for (int i = 0; i < SDL_GAMEPAD_BUTTON_COUNT; ++i)
  {
    if (SDL_GetGamepadButton(pad, static_cast<SDL_GamepadButton>(i)))
    {
      buttons |= 1u << i;
    }
  }
  x = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTX);
  y = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTY);
}

void ControllerInput::open(SDL_JoystickID id)
{
  if (gamepads.contains(id))
  {
    return;
  }
  if (auto pad = SDL_OpenGamepad(id))
  {
    try
    {
      gamepads.emplace(id, State{pad}).first->second.snapshot();
    }
    catch (...)
    {
      SDL_CloseGamepad(pad);
      throw;
    }
  }
}

void ControllerInput::close()
{
  for (auto& [id, state] : gamepads)
  {
    SDL_CloseGamepad(state.pad);
  }
  gamepads.clear();
  // Do not shut down the emulator's audio subsystem.
  SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
}

ControllerInput::ControllerInput() : owner(SDL_GetCurrentThreadID())
{
  // Flutter owns the window, so focus is enforced by the Dart session instead.
  SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
  SDL_SetHint(SDL_HINT_JOYSTICK_DIRECTINPUT, "1");
  SDL_SetHint(SDL_HINT_XINPUT_ENABLED, "1");
  if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD))
  {
    throw std::runtime_error(SDL_GetError());
  }
  int count = 0;
  auto ids = SDL_GetGamepads(&count);
  try
  {
    for (int i = 0; i < count; ++i)
    {
      open(ids[i]);
    }
  }
  catch (...)
  {
    SDL_free(ids);
    close();
    throw;
  }
  SDL_free(ids);
}

ControllerInput::~ControllerInput()
{
  close();
}

bool ControllerInput::on_owner_thread() const
{
  return SDL_GetCurrentThreadID() == owner;
}

uint32_t ControllerInput::poll()
{
  if (!on_owner_thread())
  {
    throw std::runtime_error("Controller polling must stay on the main thread");
  }
  SDL_PumpEvents();
  SDL_Event event;
  int result;
  // Drain only input events; leave audio and other subsystems' events alone.
  while ((result = SDL_PeepEvents(&event, 1, SDL_GETEVENT,
      SDL_EVENT_JOYSTICK_AXIS_MOTION, SDL_EVENT_GAMEPAD_STEAM_HANDLE_UPDATED)) > 0)
  {
    switch (event.type)
    {
      case SDL_EVENT_GAMEPAD_ADDED:
        open(event.gdevice.which);
        break;
      case SDL_EVENT_GAMEPAD_REMOVED:
        if (auto it = gamepads.find(event.gdevice.which); it != gamepads.end())
        {
          SDL_CloseGamepad(it->second.pad);
          gamepads.erase(it);
        }
        break;
      case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
      case SDL_EVENT_GAMEPAD_BUTTON_UP:
        if (auto it = gamepads.find(event.gbutton.which); it != gamepads.end() &&
            event.gbutton.button < SDL_GAMEPAD_BUTTON_COUNT)
        {
          const auto bit = 1u << event.gbutton.button;
          if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)
          {
            it->second.buttons |= bit;
          }
          else
          {
            it->second.buttons &= ~bit;
          }
        }
        break;
      case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        if (auto it = gamepads.find(event.gaxis.which); it != gamepads.end())
        {
          if (event.gaxis.axis == SDL_GAMEPAD_AXIS_LEFTX)
          {
            it->second.x = event.gaxis.value;
          }
          if (event.gaxis.axis == SDL_GAMEPAD_AXIS_LEFTY)
          {
            it->second.y = event.gaxis.value;
          }
        }
        break;
      case SDL_EVENT_GAMEPAD_REMAPPED:
        if (auto it = gamepads.find(event.gdevice.which); it != gamepads.end())
        {
          it->second.snapshot();
        }
        break;
    }
  }
  if (result < 0)
  {
    throw std::runtime_error(SDL_GetError());
  }
  uint32_t mask = 0;
  for (auto& [id, state] : gamepads)
  {
    if (SDL_GamepadConnected(state.pad))
    {
      mask |= JoypadGeneric::mapButtons(state.buttons, state.x, state.y);
    }
  }
  return mask;
}
