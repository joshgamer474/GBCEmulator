import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:gbcemulator/gbcemulator.dart';

/// SDLWindow gameplay mappings, active only while the video has focus.
class GameKeyboard extends StatefulWidget {
  const GameKeyboard({
    super.key,
    required this.enabled,
    required this.onButton,
    required this.child,
  });
  final bool enabled;
  final void Function(Button, bool) onButton;
  final Widget child;

  @override
  State<GameKeyboard> createState() => _GameKeyboardState();
}

class _GameKeyboardState extends State<GameKeyboard>
    with WidgetsBindingObserver {
  final _focus = FocusNode(debugLabel: 'Game controls');
  final _held = <Button>{};
  static final _keys = {
    LogicalKeyboardKey.keyW: Button.up,
    LogicalKeyboardKey.keyA: Button.left,
    LogicalKeyboardKey.keyS: Button.down,
    LogicalKeyboardKey.keyD: Button.right,
    LogicalKeyboardKey.keyZ: Button.a,
    LogicalKeyboardKey.keyX: Button.b,
    LogicalKeyboardKey.keyM: Button.start,
    LogicalKeyboardKey.keyN: Button.select,
  };

  void _release() {
    for (final button in _held) {
      widget.onButton(button, false);
    }
    _held.clear();
  }

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addObserver(this);
  }

  @override
  void didUpdateWidget(GameKeyboard oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (widget.enabled && !oldWidget.enabled) _focus.requestFocus();
    if (!widget.enabled && oldWidget.enabled) _release();
  }

  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    if (state != AppLifecycleState.resumed) _release();
  }

  @override
  void dispose() {
    _release();
    WidgetsBinding.instance.removeObserver(this);
    _focus.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) => Focus(
    focusNode: _focus,
    onFocusChange: (focused) {
      if (!focused) _release();
    },
    onKeyEvent: (_, event) {
      final button = _keys[event.logicalKey];
      if (!widget.enabled || button == null) return KeyEventResult.ignored;
      if (event is KeyDownEvent && _held.add(button)) {
        widget.onButton(button, true);
      } else if (event is KeyUpEvent && _held.remove(button)) {
        widget.onButton(button, false);
      }
      return KeyEventResult.handled;
    },
    child: GestureDetector(
      behavior: HitTestBehavior.opaque,
      onTap: () => _focus.requestFocus(),
      child: widget.child,
    ),
  );
}
