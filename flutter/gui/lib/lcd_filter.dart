import 'dart:ui' as ui;

import 'package:flutter/material.dart';

/// Flutter port of fishku's Authentic GBC Fast (CC0). The caller owns the image.
/// Copy this file and shaders/rgb_lcd.frag and register the shader to reuse.
class LcdFilter extends StatefulWidget {
  const LcdFilter({
    super.key,
    required this.image,
    this.enabled = true,
    this.strength = 1.0,
    this.brightness = 0.93,
    this.smoothing = 0.3,
  }) : assert(strength >= 0 && strength <= 1),
       assert(brightness >= 0 && brightness <= 1),
       assert(smoothing >= 0 && smoothing <= 1);

  final ui.Image image;
  final bool enabled;
  final double strength;

  /// Upstream brightness boost changes aperture size, rather than exposure.
  final double brightness;

  /// Spatial anti-banding smoothing, not temporal ghosting.
  final double smoothing;

  @override
  State<LcdFilter> createState() => _LcdFilterState();
}

class _LcdFilterState extends State<LcdFilter> {
  static Future<ui.FragmentProgram>? _program;
  ui.FragmentShader? _shader;

  @override
  void initState() {
    super.initState();
    _load();
  }

  Future<void> _load() async {
    try {
      final program = await (_program ??= ui.FragmentProgram.fromAsset(
        'shaders/rgb_lcd.frag',
      ));
      if (mounted) setState(() => _shader = program.fragmentShader());
    } catch (error, stack) {
      _program = null;
      FlutterError.reportError(
        FlutterErrorDetails(
          exception: error,
          stack: stack,
          context: ErrorDescription(
            'while loading the LCD shader; using raw video',
          ),
        ),
      );
    }
  }

  @override
  void dispose() {
    _shader?.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final shader = _shader;
    if (!widget.enabled || widget.strength == 0 || shader == null) {
      return RawImage(
        image: widget.image,
        fit: BoxFit.contain,
        filterQuality: FilterQuality.none,
      );
    }
    return CustomPaint(
      painter: _LcdPainter(
        image: widget.image,
        shader: shader,
        strength: widget.strength,
        brightness: widget.brightness,
        smoothing: widget.smoothing,
        pixelRatio: MediaQuery.devicePixelRatioOf(context),
      ),
      child: const SizedBox.expand(),
    );
  }
}

class _LcdPainter extends CustomPainter {
  const _LcdPainter({
    required this.image,
    required this.shader,
    required this.strength,
    required this.brightness,
    required this.smoothing,
    required this.pixelRatio,
  });

  final ui.Image image;
  final ui.FragmentShader shader;
  final double strength;
  final double brightness;
  final double smoothing;
  final double pixelRatio;

  @override
  void paint(Canvas canvas, Size size) {
    if (size.isEmpty) return;
    final source = Size(image.width.toDouble(), image.height.toDouble());
    final fitted = applyBoxFit(BoxFit.contain, source, size);
    final rect = Alignment.center.inscribe(
      fitted.destination,
      Offset.zero & size,
    );
    shader
      ..setFloat(0, rect.width)
      ..setFloat(1, rect.height)
      ..setFloat(2, source.width)
      ..setFloat(3, source.height)
      ..setFloat(4, pixelRatio)
      ..setFloat(5, strength)
      ..setFloat(6, brightness)
      ..setFloat(7, smoothing)
      ..setImageSampler(0, image);
    canvas.save();
    canvas.translate(rect.left, rect.top);
    canvas.drawRect(Offset.zero & rect.size, Paint()..shader = shader);
    canvas.restore();
  }

  @override
  bool shouldRepaint(_LcdPainter oldDelegate) =>
      image != oldDelegate.image ||
      shader != oldDelegate.shader ||
      strength != oldDelegate.strength ||
      brightness != oldDelegate.brightness ||
      smoothing != oldDelegate.smoothing ||
      pixelRatio != oldDelegate.pixelRatio;
}
