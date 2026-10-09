import 'dart:collection';

/// One frame of initial headroom smooths arrivals across display vsyncs.
/// Ownership transfers to the caller on take; dropped frames are released here.
class VideoFrameQueue<T> {
  VideoFrameQueue(this.release);
  final void Function(T) release;
  final _frames = Queue<T>();
  bool _started = false;

  void add(T frame) {
    if (_frames.length == 3) release(_frames.removeFirst());
    _frames.addLast(frame);
  }

  T? take() {
    if (!_started) {
      if (_frames.length < 2) return null;
      _started = true;
    }
    return _frames.isEmpty ? null : _frames.removeFirst();
  }

  void clear() {
    while (_frames.isNotEmpty) {
      release(_frames.removeFirst());
    }
    _started = false;
  }
}
