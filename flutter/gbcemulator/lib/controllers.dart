import 'dart:ffi';

/// SDL controller event pump. Use only on Flutter's root isolate with merged
/// UI/platform threads (Flutter 3.35+), never in an emulation worker isolate.
class GBCControllers {
  late final Pointer<Void> Function() _create;
  late final int Function(Pointer<Void>) _poll;
  late final void Function(Pointer<Void>) _destroy;
  Pointer<Void> _handle = nullptr;

  GBCControllers(DynamicLibrary library) {
    _create = library
        .lookupFunction<Pointer<Void> Function(), Pointer<Void> Function()>(
          'controllers_create',
        );
    _poll = library
        .lookupFunction<
          Int32 Function(Pointer<Void>),
          int Function(Pointer<Void>)
        >('controllers_poll');
    _destroy = library
        .lookupFunction<
          Void Function(Pointer<Void>),
          void Function(Pointer<Void>)
        >('controllers_destroy');
    _handle = _create();
    if (_handle == nullptr) {
      throw StateError(
        'Could not initialize SDL gamepads (or a controller session is already open)',
      );
    }
  }

  int poll() {
    if (_handle == nullptr) throw StateError('Controller session is closed');
    final mask = _poll(_handle);
    if (mask < 0) throw StateError('SDL controller polling failed: $mask');
    return mask;
  }

  void dispose() {
    if (_handle == nullptr) return;
    _destroy(_handle);
    _handle = nullptr;
  }
}
