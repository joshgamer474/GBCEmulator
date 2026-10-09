import 'dart:async';
import 'dart:isolate';
import 'dart:io';
import 'dart:typed_data';

import 'package:gbcemulator/gbcemulator.dart';
import 'package:flutter/foundation.dart' show kDebugMode, debugPrint;

import 'game_input.dart';

/// Runs emulation continuously; next() requests only a display snapshot.
class EmulatorSession {
  final ReceivePort _events = ReceivePort();
  final Completer<SendPort> _ready = Completer<SendPort>();
  final Completer<void> _closed = Completer<void>();
  final void Function(Uint8List) onFrame;
  final void Function(String) onError;
  SendPort? _commands;
  bool _stopping = false;
  int? nativeFrames;
  double? nativeFps;
  double _nativeSampleTime = 0;
  GBCControllers? _controllers;
  Timer? _controllerTimer;
  bool _controllersEnabled = true;
  int _lastControllerMask = 0;
  late final GameInput _input = GameInput((button, pressed) {
    if (!_stopping) _commands?.send((button.index, pressed));
  });
  final void Function(String)? onControllerError;

  EmulatorSession(this.onFrame, this.onError, {this.onControllerError});

  set controllersEnabled(bool enabled) {
    if (kDebugMode && enabled != _controllersEnabled) {
      debugPrint('Controllers: window focused=$enabled');
    }
    _controllersEnabled = enabled;
    if (!enabled) _input.release();
  }

  void _closeControllers() {
    _controllerTimer?.cancel();
    _controllerTimer = null;
    _input.controllers(0);
    _controllers?.dispose();
    _controllers = null;
  }

  void _pollControllers() {
    try {
      final mask = _controllers!.poll();
      if (kDebugMode && mask != _lastControllerMask) {
        debugPrint('Controllers: SDL mask=$mask, enabled=$_controllersEnabled');
        _lastControllerMask = mask;
      }
      _input.controllers(_controllersEnabled ? mask : 0);
    } catch (error) {
      _closeControllers();
      onControllerError?.call('Controller input unavailable: $error');
    }
  }

  Future<void> start(String library, String rom) async {
    String? startupError;
    // Initialize SDL here on Flutter's root/platform thread, before the worker
    // creates the emulator and initializes SDL audio.
    if (Platform.isWindows || Platform.isLinux || Platform.isMacOS) {
      try {
        _controllers = GBCControllers(openLibrary(library));
      } catch (error) {
        onControllerError?.call('Controller input unavailable: $error');
      }
    }
    _events.listen((dynamic event) {
      if (event is SendPort) {
        _commands = event;
        _ready.complete(event);
        if (_stopping) event.send('stop');
      } else if (event is TransferableTypedData) {
        if (!_stopping) onFrame(event.materialize().asUint8List());
      } else if (event is (String, int?, double) && event.$1 == 'frames') {
        if (event.$2 != null &&
            nativeFrames != null &&
            event.$3 > _nativeSampleTime) {
          nativeFps =
              (event.$2! - nativeFrames!) / (event.$3 - _nativeSampleTime);
        }
        nativeFrames = event.$2;
        _nativeSampleTime = event.$3;
      } else if (event is String) {
        if (!_ready.isCompleted) startupError = event;
        onError(event);
      } else if (event == null) {
        _closeControllers();
        _stopping = true;
        if (!_ready.isCompleted) {
          _ready.completeError(
            StateError(startupError ?? 'Emulator failed to start'),
          );
        }
        if (!_closed.isCompleted) _closed.complete();
        _events.close();
      }
    });
    try {
      await Isolate.spawn(_run, (
        library,
        rom,
        _events.sendPort,
      ), onExit: _events.sendPort);
      await _ready.future;
      if (!_stopping && _controllers != null) {
        _controllerTimer = Timer.periodic(
          const Duration(milliseconds: 8),
          (_) => _pollControllers(),
        );
      }
    } catch (_) {
      _closeControllers();
      _events.close();
      if (!_closed.isCompleted) _closed.complete();
      rethrow;
    }
  }

  void button(Button button, bool pressed) {
    if (!_stopping) _input.keyboard(button, pressed);
  }

  void next() {
    if (!_stopping) _commands?.send('frame');
  }

  void touchButton(Button button, bool pressed) {
    if (!_stopping) _input.touch(button, pressed);
  }

  Future<void> stop() async {
    _controllerTimer?.cancel();
    _input.release();
    _stopping = true;
    _commands?.send('stop');
    await _closed.future;
  }
}

void _run((String, String, SendPort) args) async {
  final (library, rom, output) = args;
  GBCEmulator? emulator;
  final commands = ReceivePort();
  Timer? statistics;
  try {
    emulator = GBCEmulator.create(rom, library: openLibrary(library));
    final active = emulator;
    active.set_frame_callback(() {
      try {
        output.send(TransferableTypedData.fromList([active.get_frame()]));
      } catch (error) {
        output.send(error.toString());
        commands.sendPort.send('stop');
      }
    });
    emulator.run();
    output.send(commands.sendPort);
    final clock = Stopwatch()..start();
    statistics = Timer.periodic(const Duration(seconds: 1), (_) {
      output.send((
        'frames',
        active.producedFrames,
        clock.elapsedMicroseconds / 1000000,
      ));
    });

    await for (final command in commands) {
      if (command == 'stop') break;
      if (command is (int, bool)) {
        final button = Button.values[command.$1];
        if (command.$2) {
          emulator.set_joypad_button(button);
        } else {
          emulator.release_joypad_button(button);
        }
      }
      if (command == 'frame') {
        emulator.request_frame();
      }
    }
  } catch (error) {
    output.send(error.toString());
  } finally {
    statistics?.cancel();
    emulator?.destroy();
    commands.close();
  }
}
