
## Build with Flutter (Windows)

Run:

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

## Build with Flutter (macOS)

Run:

`flutter build macos --release`.

First configure the native build **from the repository root**. On a Mac with
Homebrew dependencies:

```sh
brew install cmake sdl3 spdlog libpng libzip
cmake -S . -B build/flutter-ffi -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=13.3 \
  -DBUILD_FLUTTER_FFI=ON -DBUILD_LIB_ONLY=ON -DBUILD_UNIT_TEST=OFF \
  -DBUILD_SHARED_LIBS=OFF
```

## Build with Flutter (Linux)

TODO

## Build the native emulator for iOS with Conan 2

For iOS:

```sh
conan install . -pr:b=default -pr:h=profiles/ios --build=missing \
  -of=build/ios
conan build . -pr:b=default -pr:h=profiles/ios -of=build/ios
```

For iOS simulator:

```sh
conan install . -pr:b=default -pr:h=profiles/ios-simulator --build=missing \
  -of=build/ios-simulator
conan build . -pr:b=default -pr:h=profiles/ios-simulator \
  -of=build/ios-simulator
```