import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

/// Keyboard navigation scoped to the game area, not the surrounding text fields.
class KeyboardScrollArea extends StatefulWidget {
  const KeyboardScrollArea({
    super.key,
    required this.controller,
    required this.child,
  });
  final ScrollController controller;
  final Widget child;

  @override
  State<KeyboardScrollArea> createState() => _KeyboardScrollAreaState();
}

class _KeyboardScrollAreaState extends State<KeyboardScrollArea> {
  final _focus = FocusNode(debugLabel: 'Game library scrolling');

  @override
  void dispose() {
    _focus.dispose();
    super.dispose();
  }

  KeyEventResult _onKey(FocusNode node, KeyEvent event) {
    if (event is! KeyDownEvent && event is! KeyRepeatEvent) {
      return KeyEventResult.ignored;
    }
    final keyboard = HardwareKeyboard.instance;
    if (keyboard.isControlPressed ||
        keyboard.isAltPressed ||
        keyboard.isMetaPressed ||
        keyboard.isShiftPressed) {
      return KeyEventResult.ignored;
    }
    if (!widget.controller.hasClients) return KeyEventResult.ignored;
    final position = widget.controller.position;
    final page = position.viewportDimension * 0.9;
    final double target;
    switch (event.logicalKey) {
      case LogicalKeyboardKey.home:
        target = position.minScrollExtent;
      case LogicalKeyboardKey.end:
        target = position.maxScrollExtent;
      case LogicalKeyboardKey.pageUp:
        target = position.pixels - page;
      case LogicalKeyboardKey.pageDown:
        target = position.pixels + page;
      default:
        return KeyEventResult.ignored;
    }
    widget.controller.animateTo(
      target.clamp(position.minScrollExtent, position.maxScrollExtent),
      duration: const Duration(milliseconds: 150),
      curve: Curves.easeOut,
    );
    return KeyEventResult.handled;
  }

  @override
  Widget build(BuildContext context) => Focus(
    focusNode: _focus,
    autofocus: true,
    onKeyEvent: _onKey,
    child: Listener(
      behavior: HitTestBehavior.translucent,
      onPointerDown: (_) => _focus.requestFocus(),
      child: widget.child,
    ),
  );
}
