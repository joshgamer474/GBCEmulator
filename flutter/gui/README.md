
## Build with Flutter (Windows)

From `flutter/gui`, run:

```powershell
flutter build windows --release
```

Windows CMake automatically builds the configured native `gbcemulator_ffi` target
in Release mode and copies its DLLs into the Flutter output:

```text
build/windows/x64/runner/Release/
  GBCEmulator.exe
  flutter_windows.dll
  ...plugin DLLs
  data/
  gbcemulator_ffi.dll
  zip.dll
```

Distribute that complete folder. The optional PowerShell script below additionally
creates a versioned ZIP. Debug/profile Flutter builds also use the native Release
core for emulation speed. Conan dependencies and the native CMake configuration
are still a one-time prerequisite; they are not downloaded by this integration.
The default native build directory is the repository's `build/flutter-ffi`.
To select another configured build, set an absolute path before building:

```powershell
$env:GBC_NATIVE_BUILD_DIR = 'C:\build\gbc-ffi'
flutter build windows --release
```

## Windows Release packaging

From `flutter/gui`, after resolving Flutter dependencies and configuring the native
CMake build, run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\package-windows.ps1
```

The process-only execution-policy override is useful when Windows blocks local
scripts; it does not change the machine's policy. Close any running copy of the
app first so its EXE and emulator DLL can be rebuilt.

The script runs
`flutter build windows --release --no-pub`, and stages a fresh app folder and ZIP
under `build/packages/GBCEmulator-Windows-x64-<version>`, using the GUI's
`pubspec.yaml` version without its build number (e.g. `1.0.0+1` becomes `1.0.0`). Existing
outputs are not overwritten: move them or increment the version before repackaging.
It copies:

```text
GBCEmulator-Windows-x64-1.0.0/
  GBCEmulator.exe
  flutter_windows.dll
  *_plugin.dll
  native_assets.json         (when generated)
  data/                     (Flutter assets, shader, AOT code, ICU data)
  gbcemulator_ffi.dll
  zip.dll
  ...other native dependency DLLs, if built dynamically
```

The native CMake target copies its runtime dependencies beside the FFI DLL; the
packager copies every DLL from that Release directory. Static libraries (`.lib`),
ROMs, save files, logs, and personal settings are not packaged. Native DLLs sit
directly beside the EXE. No DLL entries need to be added to `pubspec.yaml`.

The GUI resolves the bundled library relative to `Platform.resolvedExecutable`,
so the extracted folder can be moved and launched from another working directory.
`--dart-define=GBC_LIBRARY=...` still overrides that default for development;
the packaging command builds without this override. Development runs also look
for the repository's `build/flutter-ffi/Release` output.

For a different already-configured native build or Flutter SDK:

```powershell
.\scripts\package-windows.ps1 -NativeBuildDirectory C:\build\gbc-ffi -Flutter C:\flutter\bin\flutter.bat
```

One-time native configuration, **from the repository root**, using the existing
Release Conan toolchain in this checkout:

```powershell
cmake -S . -B build/flutter-ffi -DCMAKE_TOOLCHAIN_FILE=build/generators/conan_toolchain.cmake -DBUILD_FLUTTER_FFI=ON -DBUILD_LIB_ONLY=ON -DBUILD_UNIT_TEST=OFF
```

On a new development machine, first install Flutter's Windows prerequisites,
CMake and Conan 2, install this repository's Release Conan dependencies with an
x64 MSVC profile, and use the generated toolchain path reported by Conan. Run
`flutter pub get` in `flutter/gui` before packaging. Windows Developer Mode enables
Flutter's plugin symlinks; existing local junctions in this checkout also work.

Distribute the entire extracted package, not just `GBCEmulator.exe`. Recipients need the
Microsoft Visual C++ x64 Redistributable at least as recent as your build tools:
https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist
Flutter's Windows distribution guidance:
https://docs.flutter.dev/platform-integration/windows/building

This produces a portable ZIP, not an installer or signed binary. Test it on a clean
Windows machine before distributing broadly. It does not install the VC runtime.

## Build with Flutter (macOS)

From `flutter/gui`, run `flutter build macos --release`. The Runner builds the
native Release core on every build (including debug/profile), bundles the FFI
library and its transitive dylib dependencies in `Contents/Frameworks`, rewrites
native library references to `@loader_path`, and signs the copied libraries with
the app's signing identity. System libraries are left on the system. The loader
finds the FFI library relative to the app executable, independent of the working
directory. No native library entries are needed in `pubspec.yaml`.

First configure the native build **from the repository root**. On a Mac with
Homebrew dependencies:

```sh
brew install cmake sdl3 spdlog libpng libzip
cmake -S . -B build/flutter-ffi -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=13.3 \
  -DBUILD_FLUTTER_FFI=ON -DBUILD_LIB_ONLY=ON -DBUILD_UNIT_TEST=OFF \
  -DBUILD_SHARED_LIBS=OFF
```

Alternatively use matching Release Conan dependencies and pass their generated
`conan_toolchain.cmake` with `-DCMAKE_TOOLCHAIN_FILE=...`. Use CMake 3.23 or newer
for the native build and bundling. Dependency installation and native
configuration are one-time prerequisites; the Xcode phase does not download
dependencies.

The core's libc++ formatting requires macOS 13.3 or newer. Set the native
deployment target explicitly so Xcode's environment cannot change it during
regeneration. Third-party dylibs can require a newer macOS version; build those
dependencies for your intended minimum OS when distributing the app.

Set `GBC_NATIVE_BUILD_DIR` to an absolute path to use another configured native
build. The bundler supports dylib dependencies; configure third-party native
framework dependencies as dylibs. Flutter/plugin frameworks retain their normal
Flutter embedding process.

All native dependencies must contain every architecture requested by Xcode.
Homebrew libraries normally contain only the host architecture, so the Runner
defaults to that architecture (mapping ARM hosts to Flutter's `arm64` slice) in
`macos/Runner/Configs/AppInfo.xcconfig`. For a universal app, set `ARCHS` to
`arm64 x86_64` there, use universal dependencies, and configure the native build
with `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`. The bundler reports architecture
mismatches instead of producing an app that cannot load its core.

This integration handles native building, loading, bundling, and library signing.
The macOS entitlements allow read/write access to user-selected ROM folders and
outgoing internet connections for box art. Selected folders get device-local
security-scoped bookmarks, restored before settings sync and scanning on startup.
Folders added before bookmark support must be selected once again with Add ROM
Folder. Reconnect unavailable network drives before starting the app. Distribution
notarization still needs to be configured separately. Distribute the complete
`.app`, rather than its executable alone.

Linux still needs its own runner and native dependency packaging.
