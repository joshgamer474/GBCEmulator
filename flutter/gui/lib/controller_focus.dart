import 'package:window_manager/window_manager.dart';

/// Desktop window activation is independent of Flutter's app lifecycle state.
class ControllerFocus with WindowListener {
  ControllerFocus(this.onChanged, {WindowManager? manager})
    : _manager = manager ?? windowManager;

  final void Function(bool) onChanged;
  final WindowManager _manager;
  bool _active = false;
  int _revision = 0;

  Future<void> start() async {
    _active = true;
    _manager.addListener(this);
    onChanged(false);
    final revision = _revision;
    final focused = await _manager.isFocused();
    if (_active && revision == _revision) onChanged(focused);
  }

  @override
  void onWindowFocus() => _change(true);

  @override
  void onWindowBlur() => _change(false);

  void _change(bool focused) {
    ++_revision;
    if (_active) onChanged(focused);
  }

  void dispose() {
    _active = false;
    _manager.removeListener(this);
  }
}
