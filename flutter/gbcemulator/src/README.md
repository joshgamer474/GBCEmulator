# GBCEmulator C API

`gbcemulator.h` is a C-compatible interface for Dart FFI. The implementation
links to the existing C++ core; the header does not expose C++ or SDL types.

- `create(rom_name)` uses the core constructor defaults and returns an opaque
  handle, or NULL on failure. The core still handles logging, audio, and saves.
- `destroy(handle)` releases the emulator (including the core's save writes).
- `set_joypad_button` / `release_joypad_button` accept `gbc_button` values.
- `run_next_instruction` steps once, retaining the core's audio/frame pacing.
- `get_frame` copies 160 Ã— 144 pixels into a caller-owned 92,160-byte RGBA8
  buffer. It is a snapshot of the current framebuffer, not a frame-ready signal.

Status-returning functions return 0 on success, -1 for invalid arguments, or -2
for a caught C++ exception. Creation returns NULL on exceptions. Detailed error
messages are not exposed yet. Handles must be live, and non-NULL buffers must
point to valid memory of the declared size; arbitrary pointers cannot be validated.

Serialize all calls for an emulator, including destruction. Run instruction
stepping on a worker isolate, since native calls can block for audio pacing.
Destroy each handle exactly once. Do not share a live framebuffer pointer with
Flutter; the copying API keeps the caller's buffer independent of the core.

## Native build

Enable `BUILD_FLUTTER_FFI=ON` in the existing Conan/CMake build and build the
`gbcemulator_ffi` target. Prefer a static core (`BUILD_SHARED_LIBS=OFF`) linked into
this shared wrapper, with `BUILD_LIB_ONLY=ON` and `BUILD_UNIT_TEST=OFF`.

For example, from the repository root, using existing Debug Conan dependencies:

```powershell
cmake -S . -B build/flutter-ffi -DCMAKE_TOOLCHAIN_FILE="$PWD/build/generators/conan_toolchain.cmake" -DCMAKE_CONFIGURATION_TYPES=Debug -DBUILD_FLUTTER_FFI=ON -DBUILD_LIB_ONLY=ON -DBUILD_UNIT_TEST=OFF
cmake --build build/flutter-ffi --config Debug --target gbcemulator_ffi
```

Use matching Conan dependencies for other configurations or architectures.
On Windows the build copies CMake-known runtime DLL dependencies beside the
wrapper. Ship these alongside the wrapper when integrating the Flutter app;
installing the wrapper alone does not collect its dependencies.

Dart bindings are in ../lib. Flutter build-hook integration is not implemented yet.


## Native thread execution

run(handle) starts one background thread and returns immediately. Repeated calls are idempotent. destroy requests shutdown, joins the worker, then releases the core. Do not call destroy concurrently with another C API call. Manual stepping is rejected after run. get_frame now returns the last completed frame (black before the first frame), protected by a separate mutex. Worker exceptions cause subsequent run/input/frame calls to return GBC_ERROR. All external calls on a handle must still be serialized.

## Frame notifications

Register set_frame_callback before run. request_frame arms a one-shot notification from the next setFrameUpdateMethod completion; repeated pending requests coalesce. The callback runs on the native thread while the frame mutex is held: it must only post asynchronously, never call back into the API synchronously. Dart uses NativeCallable.listener and reads get_frame after notification. destroy joins the producer before Dart closes the callable. No native buffer pointer crosses the callback boundary.

## SDL3 controller input

The Flutter GUI initializes `GBCControllers` on its root/platform thread before
starting the emulator isolate. This requires Flutter 3.35+ with the default merged
UI/platform threads; do not move these calls into `Isolate.spawn`. SDL initialization,
event pumping, and controller cleanup must run on that same main thread.

`controllers_create`, `controllers_poll`, and `controllers_destroy` expose a single
SDL gamepad event consumer independently from the emulator handle. The GUI pumps
input every 8 ms without a blocking event loop. It handles gamepad addition/removal,
button down/up, left-stick movement, and mapping changes. Connected devices are
opened at startup and closed on removal/shutdown. Input errors display a message;
keyboard emulation remains available, including when an older DLL lacks these exports.

SDL handles XInput, DirectInput, and other supported backends. The bridge no longer
calls XInput or DirectInput itself, and there is no XInput-priority restriction.
JoypadGeneric supplies shared SDL-to-Joypad mappings; JoypadXInput remains available
to existing native clients but is not polled alongside SDL in Flutter.

- D-pad / left stick: directions (stick dead zone 12000 / 32767).
- South: Game Boy A; East or West: Game Boy B.
- Start: Start; Back or left shoulder: Select.

SDL needs a gamepad mapping for a joystick to receive standardized gamepad events.
Unknown devices may need an SDL mapping via `SDL_GAMECONTROLLERCONFIG`; arbitrary
raw joystick button numbers are intentionally not treated as standardized buttons.

Flutter combines controller snapshots with keyboard state before sending changed
button edges through the existing `set_joypad_button`/`release_joypad_button` C API.
Only the emulation thread mutates the core joypad. Losing application focus clears
input, and disconnecting one controller preserves buttons held on another. The event
pump continues while unfocused to keep discovery/state current, but sends no input.
Stopping joins the emulator before releasing the SDL gamepad subsystem. It never
calls global `SDL_Quit`, which would disrupt audio.

Build the updated DLL with `cmake --build build/flutter-controllers --config Release
--target gbcemulator_ffi`. Select `build/flutter-controllers/Release/gbcemulator_ffi.dll`
in the GUI (or rebuild the DLL at your configured path), and restart the app.

For virtual-gamepad regression coverage, configure `-DBUILD_CONTROLLER_TEST=ON`,
build `controller_input_test`, then run `ctest --test-dir build/flutter-controllers
-C Release -R controller_input --output-on-failure`. This covers hot-plug, device IDs
above four, aliases, multiple controllers, disconnect releases, stick dead zones,
thread ownership, and subsystem restart. Physical hardware and macOS/Linux still
need separate testing.
