#include "../src/controller_input.h"
#include <Joypad.h>
#include <iostream>
#include <thread>

static void check(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}

static SDL_JoystickID attach()
{
  SDL_VirtualJoystickDesc desc{};
  SDL_INIT_INTERFACE(&desc);
  desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
  desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
  desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
  desc.button_mask = (1u << SDL_GAMEPAD_BUTTON_COUNT) - 1;
  desc.axis_mask = (1u << SDL_GAMEPAD_AXIS_COUNT) - 1;
  desc.name = "GBC virtual controller test";
  auto id = SDL_AttachVirtualJoystick(&desc);
  check(id != 0, "attach");
  return id;
}

int main(int argc, char** argv)
{
  try
  {
    if (argc > 1 && std::string(argv[1]) == "--diagnose")
    {
      ControllerInput input;
      int count = 0;
      auto ids = SDL_GetJoysticks(&count);
      std::cout << "SDL " << SDL_GetVersion() << "; joysticks: " << count << std::endl;
      for (int i = 0; i < count; ++i)
        std::cout << ids[i] << ": " << SDL_GetJoystickNameForID(ids[i])
          << "; mapped=" << SDL_IsGamepad(ids[i]) << std::endl;
      SDL_free(ids);
      uint32_t last = ~0u;
      for (int i = 0; i < 1000; ++i)
      {
        auto mask = input.poll();
        if (mask != last) std::cout << "buttons=" << mask << std::endl;
        last = mask;
        SDL_Delay(10);
      }
      return 0;
    }
    {
      ControllerInput input;
      // Force instance IDs above the old arbitrary limit of 4.
      for (int i = 0; i < 5; ++i) SDL_DetachVirtualJoystick(attach());
      auto first = attach();
      auto second = attach();
      check(first > 4, "high instance ID");
      auto a = SDL_OpenJoystick(first);
      auto b = SDL_OpenJoystick(second);
      input.poll();
      check(SDL_IsGamepad(first), "virtual gamepad mapping");
      SDL_SetJoystickVirtualButton(a, SDL_GAMEPAD_BUTTON_SOUTH, true);
      check(input.poll() & (1u << Joypad::A), "south -> A");
      SDL_SetJoystickVirtualButton(b, SDL_GAMEPAD_BUTTON_SOUTH, true);
      input.poll();
      SDL_SetJoystickVirtualButton(a, SDL_GAMEPAD_BUTTON_SOUTH, false);
      check(input.poll() & (1u << Joypad::A), "second pad keeps A held");
      SDL_DetachVirtualJoystick(second);
      check(!(input.poll() & (1u << Joypad::A)), "disconnect releases A");
      SDL_SetJoystickVirtualButton(a, SDL_GAMEPAD_BUTTON_EAST, true);
      SDL_SetJoystickVirtualButton(a, SDL_GAMEPAD_BUTTON_WEST, true);
      check(input.poll() & (1u << Joypad::B), "east/west -> B");
      SDL_SetJoystickVirtualButton(a, SDL_GAMEPAD_BUTTON_EAST, false);
      check(input.poll() & (1u << Joypad::B), "B alias stays held");
      SDL_SetJoystickVirtualButton(a, SDL_GAMEPAD_BUTTON_WEST, false);
      check(!(input.poll() & (1u << Joypad::B)), "B aliases released");
      SDL_SetJoystickVirtualAxis(a, SDL_GAMEPAD_AXIS_LEFTX, -20000);
      check(input.poll() & (1u << Joypad::LEFT), "stick left");
      SDL_SetJoystickVirtualAxis(a, SDL_GAMEPAD_AXIS_LEFTX, -10000);
      check(!(input.poll() & (1u << Joypad::LEFT)), "stick dead zone");
      SDL_SetJoystickVirtualButton(a, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, true);
      check(input.poll() & (1u << Joypad::SELECT), "shoulder -> select");
      bool rejected = false;
      std::thread other([&] { try { input.poll(); } catch (...) { rejected = true; } });
      other.join();
      check(rejected, "wrong-thread polling rejected");
      SDL_DetachVirtualJoystick(first);
      check(input.poll() == 0, "all disconnected");
      SDL_CloseJoystick(a);
      SDL_CloseJoystick(b);
    }
    check((SDL_WasInit(SDL_INIT_GAMEPAD) & SDL_INIT_GAMEPAD) == 0, "subsystem released");
    { ControllerInput input; check(input.poll() == 0, "restart"); }
    std::cout << "SDL virtual controller checks passed\n";
    return 0;
  }
  catch (const std::exception& error)
  {
    std::cerr << error.what() << ": " << SDL_GetError() << '\n';
    return 1;
  }
}

