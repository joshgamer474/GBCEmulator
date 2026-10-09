import 'dart:math' as math;

import 'package:flutter/material.dart';
import 'package:gbcemulator/gbcemulator.dart';

/// Independent pointers allow movement and multiple buttons at the same time.
class MobileControls extends StatefulWidget {
  const MobileControls({
    super.key,
    required this.enabled,
    required this.onButton,
  });
  final bool enabled;
  final void Function(Button, bool) onButton;

  @override
  State<MobileControls> createState() => _MobileControlsState();
}

class _MobileControlsState extends State<MobileControls>
    with WidgetsBindingObserver {
  final _held = <Button>{};
  final _pointers = <Button, Set<int>>{};
  int? _padPointer;
  Offset _stick = Offset.zero;

  void _press(Button button, bool pressed) {
    final changed = pressed ? _held.add(button) : _held.remove(button);
    if (changed) widget.onButton(button, pressed);
  }

  void _release() {
    for (final button in _held.toList()) {
      _press(button, false);
    }
    _pointers.clear();
    _padPointer = null;
    _stick = Offset.zero;
  }

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addObserver(this);
  }

  @override
  void didUpdateWidget(MobileControls oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (!widget.enabled) _release();
  }

  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    if (state != AppLifecycleState.resumed) setState(_release);
  }

  @override
  void dispose() {
    _release();
    WidgetsBinding.instance.removeObserver(this);
    super.dispose();
  }

  void _movePad(Offset position, double size) {
    final radius = size * .30;
    var delta = position - Offset(size / 2, size / 2);
    if (delta.distance > radius) delta = delta / delta.distance * radius;
    setState(() {
      _stick = delta;
      final active = delta.distance > radius * .22;
      final threshold = math.max(delta.dx.abs(), delta.dy.abs()) * .45;
      _press(Button.left, active && delta.dx < -threshold);
      _press(Button.right, active && delta.dx > threshold);
      _press(Button.up, active && delta.dy < -threshold);
      _press(Button.down, active && delta.dy > threshold);
    });
  }

  Widget _pad(double size) => Semantics(
    label: 'Directional pad',
    child: Listener(
      key: const ValueKey('touch-dpad'),
      behavior: HitTestBehavior.opaque,
      onPointerDown: (event) {
        if (!widget.enabled || _padPointer != null) return;
        _padPointer = event.pointer;
        _movePad(event.localPosition, size);
      },
      onPointerMove: (event) {
        if (_padPointer == event.pointer) _movePad(event.localPosition, size);
      },
      onPointerUp: (event) => _endPad(event.pointer),
      onPointerCancel: (event) => _endPad(event.pointer),
      child: SizedBox.square(
        dimension: size,
        child: Stack(
          children: [
            Positioned.fill(
              child: DecoratedBox(
                decoration: BoxDecoration(
                  shape: BoxShape.circle,
                  color: Colors.white.withValues(alpha: .10),
                  border: Border.all(color: Colors.white38, width: 2),
                ),
                child: const Icon(Icons.add, color: Colors.white38, size: 64),
              ),
            ),
            AnimatedPositioned(
              duration: Duration(milliseconds: _padPointer == null ? 120 : 30),
              curve: Curves.easeOut,
              left: size * .31 + _stick.dx,
              top: size * .31 + _stick.dy,
              child: Container(
                key: const ValueKey('touch-stick'),
                width: size * .38,
                height: size * .38,
                decoration: BoxDecoration(
                  shape: BoxShape.circle,
                  color: _padPointer == null
                      ? Colors.white38
                      : Colors.indigoAccent,
                  border: Border.all(color: Colors.white54),
                ),
              ),
            ),
          ],
        ),
      ),
    ),
  );

  void _endPad(int pointer) {
    if (_padPointer != pointer) return;
    setState(() {
      _padPointer = null;
      _stick = Offset.zero;
      for (final button in [
        Button.up,
        Button.down,
        Button.left,
        Button.right,
      ]) {
        _press(button, false);
      }
    });
  }

  Widget _button(
    Button button,
    String label,
    double size, {
    bool pill = false,
  }) {
    void end(int pointer) {
      final pointers = _pointers[button];
      if (pointers == null || !pointers.remove(pointer)) return;
      setState(() => _press(button, pointers.isNotEmpty));
    }

    return Semantics(
      label: label,
      button: true,
      enabled: widget.enabled,
      child: Listener(
        key: ValueKey('touch-${button.name}'),
        behavior: HitTestBehavior.opaque,
        onPointerDown: (event) {
          if (!widget.enabled) return;
          (_pointers[button] ??= {}).add(event.pointer);
          setState(() => _press(button, true));
        },
        onPointerUp: (event) => end(event.pointer),
        onPointerCancel: (event) => end(event.pointer),
        child: AnimatedContainer(
          duration: const Duration(milliseconds: 60),
          width: pill ? 84 : size,
          height: pill ? 44 : size,
          alignment: Alignment.center,
          decoration: BoxDecoration(
            color: _held.contains(button)
                ? Colors.indigoAccent
                : Colors.white24,
            borderRadius: BorderRadius.circular(pill ? 22 : size / 2),
            border: Border.all(color: Colors.white54, width: 2),
          ),
          child: Text(
            label,
            style: TextStyle(
              color: Colors.white,
              fontWeight: FontWeight.bold,
              fontSize: pill ? 12 : 24,
            ),
          ),
        ),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    final compact =
        MediaQuery.sizeOf(context).width > MediaQuery.sizeOf(context).height;
    return SafeArea(
      top: false,
      child: Padding(
        padding: const EdgeInsets.fromLTRB(16, 0, 16, 8),
        child: LayoutBuilder(
          builder: (context, constraints) {
            final size = math.min(
              compact ? 144.0 : 164.0,
              constraints.maxWidth * .50,
            );
            final buttonSize = math.min(64.0, size * .46);
            return Container(
              height: math.min(
                constraints.maxHeight,
                math.max(
                  size + 44,
                  MediaQuery.sizeOf(context).height * (compact ? .70 : .34),
                ),
              ),
              child: Opacity(
                opacity: widget.enabled ? 1 : .4,
                child: Column(
                  children: [
                    Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: [
                        // DPAD UI button
                        _pad(size),
                        // A and B UI buttons
                        Row(
                          children: [
                            Padding(
                              padding: const EdgeInsets.only(top: 28),
                              child: _button(Button.b, 'B', buttonSize),
                            ),
                            const SizedBox(width: 12),
                            Padding(
                              padding: const EdgeInsets.only(bottom: 28),
                              child: _button(Button.a, 'A', buttonSize),
                            ),
                          ],
                        ),
                      ],
                    ),
                    Expanded(child: Container()),
                    // Start and Select UI buttons
                    Row(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        _button(Button.select, '', 24, pill: true),
                        const SizedBox(width: 16),
                        _button(Button.start, '', 24, pill: true),
                      ],
                    ),
                  ],
                ),
              ),
            );
          },
        ),
      ),
    );
  }
}
