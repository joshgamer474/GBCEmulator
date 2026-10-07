import 'dart:io';

String defaultNativeLibraryPath() {
  const override = String.fromEnvironment('GBC_LIBRARY');
  if (override.isNotEmpty) return override;

  final filename = Platform.isWindows
      ? 'gbcemulator_ffi.dll'
      : Platform.isMacOS
      ? 'libgbcemulator_ffi.dylib'
      : 'libgbcemulator_ffi.so';
  final executableDirectory = File(Platform.resolvedExecutable).parent.path;
  final bundled = File('$executableDirectory/$filename');
  if (bundled.existsSync()) return bundled.path;

  // Keep flutter run convenient when launched from flutter/gui or the repo root.
  for (final root in [Directory.current.path, '${Directory.current.path}/../..']) {
    final development = File('$root/build/flutter-ffi/Release/$filename');
    if (development.existsSync()) return development.absolute.path;
  }
  return bundled.path;
}
