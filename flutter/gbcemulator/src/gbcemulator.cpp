#include "gbcemulator.h"
#include "controller_input.h"

#include <atomic>
#include <mutex>
#include <thread>

#include "../../../src/GBCEmulator.h"

struct gbc_handle
{
  GBCEmulator emulator;
  std::mutex core_mutex;
  std::mutex frame_mutex;
  std::array<SDL_Color, SCREEN_PIXEL_TOTAL> frame{};
  std::atomic<uint32_t> input_buttons{0};
  uint32_t applied_buttons = 0;
  std::atomic<bool> stopping{false};
  std::atomic<bool> failed{false};
  std::thread thread;
  gbc_frame_callback callback = nullptr;
  bool frame_requested = false;

  explicit gbc_handle(const char* rom_name) : emulator(rom_name)
  {
    emulator.setFrameUpdateMethod([this](auto completed_frame)
    {
      std::lock_guard lock(frame_mutex);
      frame = completed_frame;
      if (callback && frame_requested)
      {
        frame_requested = false;
        callback(); // NativeCallable.listener posts to Dart without waiting.
      }
    });
  }

  ~gbc_handle()
  {
    stopping = true;
    if (thread.joinable()) thread.join();
  }
};

namespace {
constexpr Joypad::BUTTON buttons[] =
{
  Joypad::DOWN, Joypad::UP, Joypad::LEFT, Joypad::RIGHT,
  Joypad::START, Joypad::SELECT, Joypad::B, Joypad::A
};

template <typename Action>
int32_t invoke(gbc_handle* handle, Action action) noexcept
{
  if (!handle) return GBC_INVALID_ARGUMENT;
  try
  {
    std::lock_guard lock(handle->core_mutex);
    if (handle->failed) return GBC_ERROR;
    action(handle->emulator);
    return GBC_OK;
  } catch (...)
  {
    return GBC_ERROR;
  }
}
}

gbc_handle* GBC_CALL create(const char* rom_name)
{
  if (!rom_name || !*rom_name) return nullptr;
  try
  {
    return new gbc_handle(rom_name);
  }
  catch (...)
  {
    return nullptr;
  }
}

int32_t GBC_CALL set_frame_callback(gbc_handle* handle, gbc_frame_callback callback)
{
  if (!handle || handle->thread.joinable()) return GBC_INVALID_ARGUMENT;
  std::lock_guard lock(handle->frame_mutex);
  handle->callback = callback;
  return GBC_OK;
}

int32_t GBC_CALL request_frame(gbc_handle* handle)
{
  if (!handle) return GBC_INVALID_ARGUMENT;
  std::lock_guard lock(handle->frame_mutex);
  if (handle->failed) return GBC_ERROR;
  if (!handle->callback) return GBC_INVALID_ARGUMENT;
  handle->frame_requested = true;
  return GBC_OK;
}

void GBC_CALL destroy(gbc_handle* handle)
{
  delete handle;
}

void apply_buttons(gbc_handle* handle)
{
  const auto combined = handle->input_buttons.load();
  const auto changed = combined ^ handle->applied_buttons;
  for (int i = 0; i < 8; ++i)
  {
    if (!(changed & (1u << i))) continue;
    if (combined & (1u << i)) handle->emulator.set_joypad_button(buttons[i]);
    else handle->emulator.release_joypad_button(buttons[i]);
  }
  handle->applied_buttons = combined;
}

void _run(gbc_handle* handle)
{
  try
  {
    while (!handle->stopping)
    {
      apply_buttons(handle);
      for (int i = 0; i < 2560 && !handle->stopping; ++i)
        handle->emulator.runNextInstruction();
    }
  }
  catch (...)
  {
    std::lock_guard lock(handle->frame_mutex);
    handle->failed = true;
    if (handle->callback && handle->frame_requested)
    {
      handle->frame_requested = false;
      handle->callback(); // Wake Dart so get_frame reports the failure.
    }
  }
}

int32_t GBC_CALL run(gbc_handle* handle)
{
  if (!handle)
  {
    return GBC_INVALID_ARGUMENT;
  }

  if (handle->failed)
  {
    return GBC_ERROR;
  }

  if (handle->thread.joinable())
  {
    return GBC_OK;
  }

  try
  {
    handle->thread = std::thread(_run, handle);
    return GBC_OK;
  }
  catch (...)
  {
    return GBC_ERROR;
  }
}

int32_t GBC_CALL set_joypad_button(gbc_handle* handle, int32_t button)
{
  if (button < GBC_DOWN || button > GBC_A) return GBC_INVALID_ARGUMENT;
  if (!handle) return GBC_INVALID_ARGUMENT;
  if (handle->failed) return GBC_ERROR;
  handle->input_buttons.fetch_or(1u << button);
  return GBC_OK;
}

int32_t GBC_CALL release_joypad_button(gbc_handle* handle, int32_t button)
{
  if (!handle || button < GBC_DOWN || button > GBC_A) return GBC_INVALID_ARGUMENT;
  if (handle->failed) return GBC_ERROR;
  handle->input_buttons.fetch_and(~(1u << button));
  return GBC_OK;
}
int32_t GBC_CALL run_next_instruction(gbc_handle* handle)
{
  if (!handle || handle->thread.joinable()) return GBC_INVALID_ARGUMENT;
  apply_buttons(handle);
  return invoke(handle, [](GBCEmulator& emulator)
  {
    emulator.runNextInstruction();
  });
}

int32_t GBC_CALL get_frame(gbc_handle* handle, uint8_t* rgba, size_t length)
{
  if (!handle || !rgba || length < SCREEN_PIXEL_TOTAL * 4) return GBC_INVALID_ARGUMENT;
  if (handle->failed) return GBC_ERROR;
  try
  {
    // A completed-frame copy avoids blocking on CPU execution or audio pacing.
    std::lock_guard lock(handle->frame_mutex);
    for (size_t i = 0; i < handle->frame.size(); ++i)
    {
      rgba[i * 4] = handle->frame[i].r;
      rgba[i * 4 + 1] = handle->frame[i].g;
      rgba[i * 4 + 2] = handle->frame[i].b;
      rgba[i * 4 + 3] = 255;
    }
    return GBC_OK;
  } catch (...)
  {
    return GBC_ERROR;
  }
}



// A single event consumer per process. Called on Flutter's root/platform thread.
struct gbc_controllers { ControllerInput input; };
static gbc_controllers* active_controllers = nullptr;

gbc_controllers* GBC_CALL controllers_create()
{
  if (active_controllers) return nullptr;
  try { return active_controllers = new gbc_controllers; }
  catch (...) { return nullptr; }
}

int32_t GBC_CALL controllers_poll(gbc_controllers* controllers)
{
  if (!controllers || controllers != active_controllers) return GBC_INVALID_ARGUMENT;
  try { return static_cast<int32_t>(controllers->input.poll()); }
  catch (...) { return GBC_ERROR; }
}

void GBC_CALL controllers_destroy(gbc_controllers* controllers)
{
  if (!controllers || controllers != active_controllers ||
      !controllers->input.on_owner_thread()) return;
  delete controllers;
  active_controllers = nullptr;
}
