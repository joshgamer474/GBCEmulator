#!/bin/sh
set -eu

# Xcode's PATH does not normally include Homebrew.
export PATH="/opt/homebrew/bin:/usr/local/bin:$PATH"
repository_root="$(cd "$PROJECT_DIR/../../.." && pwd)"
native_build="${GBC_NATIVE_BUILD_DIR:-$repository_root/build/flutter-ffi}"
if [ ! -f "$native_build/CMakeCache.txt" ]; then
  echo "error: Configure $native_build with BUILD_FLUTTER_FFI=ON and Release dependencies first. See flutter/gui/README.md." >&2
  exit 1
fi
cmake --build "$native_build" --config Release --target gbcemulator_ffi --parallel 4
cmake "-DGBC_MANIFEST=$native_build/gbc-ffi-Release.txt" \
  "-DGBC_FRAMEWORKS=$TARGET_BUILD_DIR/$FRAMEWORKS_FOLDER_PATH" \
  "-DGBC_ARCHS=$ARCHS" \
  "-DGBC_SIGNING_ALLOWED=${CODE_SIGNING_ALLOWED:-YES}" \
  "-DGBC_SIGN_IDENTITY=${EXPANDED_CODE_SIGN_IDENTITY:--}" \
  -P "$PROJECT_DIR/../scripts/bundle-macos-native.cmake"
