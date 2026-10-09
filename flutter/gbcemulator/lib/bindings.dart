// Names intentionally match the C API.
// ignore_for_file: non_constant_identifier_names

import 'dart:ffi';

typedef FrameCallback = Void Function();

final class GBCHandle extends Opaque {}

/// Low-level bindings to src/gbcemulator.h. Callers own all native memory.
class GBCBindings {
  GBCBindings(DynamicLibrary library)
    : create = library
          .lookupFunction<
            Pointer<GBCHandle> Function(Pointer<Char>),
            Pointer<GBCHandle> Function(Pointer<Char>)
          >('create'),
      destroy = library
          .lookupFunction<
            Void Function(Pointer<GBCHandle>),
            void Function(Pointer<GBCHandle>)
          >('destroy'),
      set_joypad_button = library
          .lookupFunction<
            Int32 Function(Pointer<GBCHandle>, Int32),
            int Function(Pointer<GBCHandle>, int)
          >('set_joypad_button'),
      release_joypad_button = library
          .lookupFunction<
            Int32 Function(Pointer<GBCHandle>, Int32),
            int Function(Pointer<GBCHandle>, int)
          >('release_joypad_button'),
      set_frame_callback = library
          .lookupFunction<
            Int32 Function(
              Pointer<GBCHandle>,
              Pointer<NativeFunction<FrameCallback>>,
            ),
            int Function(
              Pointer<GBCHandle>,
              Pointer<NativeFunction<FrameCallback>>,
            )
          >('set_frame_callback'),
      request_frame = library
          .lookupFunction<
            Int32 Function(Pointer<GBCHandle>),
            int Function(Pointer<GBCHandle>)
          >('request_frame'),
      run = library
          .lookupFunction<
            Int32 Function(Pointer<GBCHandle>),
            int Function(Pointer<GBCHandle>)
          >('run'),
      run_next_instruction = library
          .lookupFunction<
            Int32 Function(Pointer<GBCHandle>),
            int Function(Pointer<GBCHandle>)
          >('run_next_instruction'),
      get_frame = library
          .lookupFunction<
            Int32 Function(Pointer<GBCHandle>, Pointer<Uint8>, Size),
            int Function(Pointer<GBCHandle>, Pointer<Uint8>, int)
          >('get_frame') {
    // Keep Dart compatible with native libraries built before diagnostics.
    _frameCount = library.providesSymbol('gbc_frame_count')
        ? library.lookupFunction<
            Uint64 Function(Pointer<GBCHandle>),
            int Function(Pointer<GBCHandle>)
          >('gbc_frame_count')
        : null;
    last_error = library.providesSymbol('gbc_last_error')
        ? library.lookupFunction<
            Pointer<Char> Function(),
            Pointer<Char> Function()
          >('gbc_last_error')
        : null;
  }

  late final Pointer<Char> Function()? last_error;

  int? frameCount(Pointer<GBCHandle> handle) => _frameCount?.call(handle);
  late final int Function(Pointer<GBCHandle>)? _frameCount;

  final Pointer<GBCHandle> Function(Pointer<Char>) create;
  final void Function(Pointer<GBCHandle>) destroy;
  final int Function(Pointer<GBCHandle>, int) set_joypad_button;
  final int Function(Pointer<GBCHandle>, int) release_joypad_button;
  final int Function(Pointer<GBCHandle>, Pointer<NativeFunction<FrameCallback>>)
  set_frame_callback;
  final int Function(Pointer<GBCHandle>) request_frame;
  final int Function(Pointer<GBCHandle>) run;
  final int Function(Pointer<GBCHandle>) run_next_instruction;
  final int Function(Pointer<GBCHandle>, Pointer<Uint8>, int) get_frame;
}
