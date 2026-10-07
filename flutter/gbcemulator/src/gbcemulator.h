#pragma once

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
#if defined(GBC_FFI_BUILD)
#define GBC_API __declspec(dllexport)
#else
#define GBC_API __declspec(dllimport)
#endif
#define GBC_CALL __cdecl
#else
#define GBC_API __attribute__((visibility("default")))
#define GBC_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gbc_handle gbc_handle;
typedef struct gbc_controllers gbc_controllers;
// One controller event consumer. Create/poll/destroy on the host main thread,
// before creating an emulator. Poll returns a gbc_button bitmask or negative status.
GBC_API gbc_controllers* GBC_CALL controllers_create(void);
GBC_API int32_t GBC_CALL controllers_poll(gbc_controllers* controllers);
GBC_API void GBC_CALL controllers_destroy(gbc_controllers* controllers);
typedef void (GBC_CALL *gbc_frame_callback)(void);
// Register before run. Callback is asynchronous/thread-safe and must not block.
GBC_API int32_t GBC_CALL set_frame_callback(gbc_handle* handle, gbc_frame_callback callback);
// Arm one notification for the next completed frame. Repeated requests coalesce.
GBC_API int32_t GBC_CALL request_frame(gbc_handle* handle);

enum gbc_button {
  GBC_DOWN = 0, GBC_UP, GBC_LEFT, GBC_RIGHT,
  GBC_START, GBC_SELECT, GBC_B, GBC_A
};

enum gbc_status {
  GBC_OK = 0,
  GBC_INVALID_ARGUMENT = -1,
  GBC_ERROR = -2
};

GBC_API gbc_handle* GBC_CALL create(const char* rom_name);
GBC_API void GBC_CALL destroy(gbc_handle* handle);
GBC_API int32_t GBC_CALL run(gbc_handle* handle);

GBC_API int32_t GBC_CALL set_joypad_button(gbc_handle* handle, int32_t button);
GBC_API int32_t GBC_CALL release_joypad_button(gbc_handle* handle, int32_t button);
GBC_API int32_t GBC_CALL run_next_instruction(gbc_handle* handle);

GBC_API int32_t GBC_CALL get_frame(gbc_handle* handle, uint8_t* rgba, size_t length);

#ifdef __cplusplus
}
#endif


