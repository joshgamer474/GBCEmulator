import 'dart:ui';

/// Bounded rolling measurements; reporting does not rebuild UI per frame.
class FrameTimingStats {
  final _frames = <FrameTiming>[];

  void add(List<FrameTiming> frames) {
    _frames.addAll(frames);
    if (_frames.length > 240) _frames.removeRange(0, _frames.length - 240);
  }

  void clear() => _frames.clear();

  String get summary {
    if (_frames.isEmpty) return 'Flutter timings: collecting…';
    String metric(Duration Function(FrameTiming) duration) {
      final values =
          _frames.map((frame) => duration(frame).inMicroseconds / 1000).toList()
            ..sort();
      final mean = values.reduce((a, b) => a + b) / values.length;
      final p95 =
          values[((values.length * .95).ceil() - 1).clamp(
            0,
            values.length - 1,
          )];
      return '${mean.toStringAsFixed(1)} / ${p95.toStringAsFixed(1)} ms';
    }

    return 'Flutter timings (avg / p95)\n'
        'UI: ${metric((frame) => frame.buildDuration)}\n'
        'Raster: ${metric((frame) => frame.rasterDuration)}\n'
        'Vsync wait: ${metric((frame) => frame.vsyncOverhead)}\n'
        'Total: ${metric((frame) => frame.totalSpan)}';
  }
}
