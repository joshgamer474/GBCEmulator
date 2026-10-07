#pragma once

#include <SDL3/SDL.h>
#include <cstdint>
#include <unordered_map>

// All operations belong to the host's main/platform thread, never the CPU worker.
class ControllerInput
{
  struct State
  {
    SDL_Gamepad* pad;
    uint32_t buttons = 0;
    int x = 0, y = 0;

    void snapshot();
  };

  std::unordered_map<SDL_JoystickID, State> gamepads;
  SDL_ThreadID owner;

  void open(SDL_JoystickID id);
  void close();

public:
  ControllerInput();
  ~ControllerInput();
  ControllerInput(const ControllerInput&) = delete;
  ControllerInput& operator=(const ControllerInput&) = delete;

  bool on_owner_thread() const;
  uint32_t poll();
};
