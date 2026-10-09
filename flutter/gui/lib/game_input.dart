import 'package:gbcemulator/gbcemulator.dart';

/// Keep input sources independent so releasing one cannot cancel another.
class GameInput {
  GameInput(this.onButton);
  final void Function(Button, bool) onButton;
  int _keyboard = 0;
  int _controllers = 0;
  int _touch = 0;
  int _sent = 0;

  void keyboard(Button button, bool pressed) {
    if (pressed) {
      _keyboard |= 1 << button.index;
    } else {
      _keyboard &= ~(1 << button.index);
    }
    _flush();
  }

  void controllers(int mask) {
    _controllers = mask & 0xff;
    _flush();
  }

  void touch(Button button, bool pressed) {
    if (pressed) {
      _touch |= 1 << button.index;
    } else {
      _touch &= ~(1 << button.index);
    }
    _flush();
  }

  void release() {
    _keyboard = 0;
    _controllers = 0;
    _touch = 0;
    _flush();
  }

  void _flush() {
    final combined = _keyboard | _controllers | _touch;
    final changed = combined ^ _sent;
    for (final button in Button.values) {
      final bit = 1 << button.index;
      if (changed & bit != 0) onButton(button, combined & bit != 0);
    }
    _sent = combined;
  }
}
