import 'dart:ffi';
import 'dart:io';

import 'package:ffi/ffi.dart';

/// Loads a native library and, on Windows, dependencies beside that library.
DynamicLibrary openLibrary(String path) {
  final file = File(path).absolute;
  if (!file.existsSync()) {
    if (Platform.isIOS) {
      throw StateError(
        'The iOS native emulator is missing: ${file.path}. '
        'Build gbcemulator_ffi and its dependencies for iOS and embed the '
        'framework in Runner. A macOS dylib cannot be used on iOS.',
      );
    }
    throw ArgumentError('Native library does not exist: ${file.path}');
  }

  if (!Platform.isWindows) {
    return DynamicLibrary.open(file.path);
  }

  // Windows load
  final kernel = DynamicLibrary.open('kernel32.dll');
  final load = kernel
      .lookupFunction<
        Pointer<Void> Function(Pointer<Utf16>, Pointer<Void>, Uint32),
        Pointer<Void> Function(Pointer<Utf16>, Pointer<Void>, int)
      >('LoadLibraryExW');
  final error = kernel.lookupFunction<Uint32 Function(), int Function()>(
    'GetLastError',
  );
  final free = kernel
      .lookupFunction<
        Int32 Function(Pointer<Void>),
        int Function(Pointer<Void>)
      >('FreeLibrary');
  final nativePath = file.path.replaceAll('/', r'\');
  final name = nativePath.toNativeUtf16();
  try {
    // LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS.
    // Scope dependency lookup to this load; do not change process search paths.
    final module = load(name, nullptr, 0x100 | 0x1000);
    if (module == nullptr) {
      final code = error();
      throw ArgumentError(
        'Failed to load ${file.path} (Windows error $code). '
        'The file exists; check its dependent DLLs and runtime libraries.',
      );
    }
    try {
      // Dart acquires its own reference to the already-loaded module.
      return DynamicLibrary.open(nativePath);
    } finally {
      free(module);
    }
  } finally {
    calloc.free(name);
  }
}
