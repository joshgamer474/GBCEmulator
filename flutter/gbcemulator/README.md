# GBCEmulator Dart FFI bindings

`lib/bindings.dart` contains handwritten bindings matching the six exports in
`src/gbcemulator.h`. `lib/gbcemulator.dart` adds handle and buffer ownership,
button values, status checks, and independent RGBA snapshots.

Build the native library using [the native build instructions](src/README.md).
Then supply the library explicitly:

```dart
import 'dart:ffi';
import 'package:gbcemulator/gbcemulator.dart';

final emulator = GbcEmulator.create(
  'game.gb',
  library: openLibrary('/absolute/path/to/gbcemulator_ffi.dll'),
);
try {
  emulator.set_joypad_button(Button.a);
  emulator.run_next_instruction();
  emulator.release_joypad_button(Button.a);
  final rgba = emulator.get_frame(); // 160 x 144 RGBA8, owned by Dart.
} finally {
  emulator.destroy();
}
```

Use the corresponding `.so` or `.dylib` on Linux/macOS. Native builds on those
platforms have not been verified. On Windows, make dependency DLLs discoverable:
for local Dart runs, openLibrary searches beside the requested DLL;
for a packaged Flutter app, bundle them beside the executable.