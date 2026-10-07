import 'dart:ffi';
import 'dart:typed_data';

import 'package:ffi/ffi.dart';

import 'bindings.dart';

export 'bindings.dart';
export 'loader.dart';
export 'controllers.dart';

/// Values match gbc_button in the C header.
enum Button { down, up, left, right, start, select, b, a }

class GBCEmulator {
  static const frame_width = 160;
  static const frame_height = 144;
  static const frame_bytes = frame_width * frame_height * 4;

  final GBCBindings _bindings;
  Pointer<GBCHandle> _handle;
  final Pointer<Uint8> _frame;
  NativeCallable<FrameCallback>? _callback;

  GBCEmulator._(this._bindings, this._handle, this._frame);

  /// Load a built native library explicitly, then create an emulator for [rom].
  factory GBCEmulator.create(String rom, {required DynamicLibrary library}) {
    if (rom.isEmpty || rom.contains('\u0000')) {
      throw ArgumentError.value(
        rom,
        'rom',
        'Must be a nonempty path without NUL',
      );
    }
    final bindings = GBCBindings(library);
    final name = rom.toNativeUtf8();
    try {
      final handle = bindings.create(name.cast<Char>());
      if (handle == nullptr) {
        throw StateError('Could not create emulator for "$rom"');
      }
      try {
        return GBCEmulator._(bindings, handle, calloc<Uint8>(frame_bytes));
      } catch (_) {
        bindings.destroy(handle);
        rethrow;
      }
    } finally {
      calloc.free(name);
    }
  }

  void destroy() {
    if (_handle == nullptr) {
      return;
    }
    _bindings.destroy(_handle);
    _handle = nullptr;
    // Native thread is joined before its callback trampoline is released.
    _callback?.close();
    _callback = null;
    calloc.free(_frame);
  }

  void _check_alive() {
    if (_handle == nullptr) {
      throw StateError('Emulator has been destroyed');
    }
  }

  void _check_status(String operation, int status) {
    if (status != 0) {
      throw StateError('$operation failed with native status $status');
    }
  }

  void set_joypad_button(Button button) {
    _check_alive();
    _check_status(
      'set_joypad_button',
      _bindings.set_joypad_button(_handle, button.index),
    );
  }

  void release_joypad_button(Button button) {
    _check_alive();
    _check_status(
      'release_joypad_button',
      _bindings.release_joypad_button(_handle, button.index),
    );
  }

  /// Starts continuous native execution. destroy() stops and joins the thread.
  /// Register once before run; request_frame arms each frame notification.
  void set_frame_callback(void Function() onFrame) {
    _check_alive();
    if (_callback != null)
      throw StateError('Frame callback already registered');
    final callback = NativeCallable<FrameCallback>.listener(() {
      if (_handle != nullptr) onFrame();
    });
    try {
      _check_status(
        'set_frame_callback',
        _bindings.set_frame_callback(_handle, callback.nativeFunction),
      );
      _callback = callback;
    } catch (_) {
      callback.close();
      rethrow;
    }
  }

  void request_frame() {
    _check_alive();
    _check_status('request_frame', _bindings.request_frame(_handle));
  }

  void run() {
    _check_alive();
    _check_status('run', _bindings.run(_handle));
  }

  void run_next_instruction() {
    _check_alive();
    _check_status(
      'run_next_instruction',
      _bindings.run_next_instruction(_handle),
    );
  }

  Uint8List get_frame() {
    _check_alive();
    _check_status(
      'get_frame',
      _bindings.get_frame(_handle, _frame, frame_bytes),
    );
    return Uint8List.fromList(_frame.asTypedList(frame_bytes));
  }
}
