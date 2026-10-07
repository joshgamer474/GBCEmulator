import 'dart:async';
import 'dart:io';
import 'dart:ffi';
import 'dart:typed_data';
import 'dart:ui' as ui;

import 'package:flutter_test/flutter_test.dart';
import 'package:gui/emulator_session.dart';

void main() {
  test(
    'repeated sessions deliver video and release their SDL audio subsystem',
    () async {
      final library = DynamicLibrary.open(Platform.environment['GBC_LIBRARY']!);
      final wasInit = library
          .lookupFunction<Uint32 Function(Uint32), int Function(int)>(
            'SDL_WasInit',
          );
      const audio = 0x10; // SDL_INIT_AUDIO
      expect(wasInit(audio), 0);
      for (var cycle = 0; cycle < 3; cycle++) {
        final frame = Completer<void>();
        final session = EmulatorSession(
          (_) {
            if (!frame.isCompleted) frame.complete();
          },
          (error) {
            if (!frame.isCompleted) frame.completeError(StateError(error));
          },
        );
        try {
          await session.start(
            Platform.environment['GBC_LIBRARY']!,
            Platform.environment['GBC_ROM']!,
          );
          session.next();
          await frame.future.timeout(const Duration(seconds: 5));
        } finally {
          await session.stop().timeout(const Duration(seconds: 5));
        }
        expect(
          wasInit(audio),
          0,
          reason: 'Audio must be released after cycle $cycle',
        );
      }
    },
    skip:
        Platform.environment['GBC_LIBRARY'] == null ||
        Platform.environment['GBC_ROM'] == null,
  );
  test(
    'continuous worker can stop without any display requests',
    () async {
      var snapshots = 0;
      final errors = <String>[];
      final session = EmulatorSession((_) => snapshots++, errors.add);
      try {
        await session.start(
          Platform.environment['GBC_LIBRARY']!,
          Platform.environment['GBC_ROM']!,
        );
        await Future<void>.delayed(const Duration(milliseconds: 250));
        expect(snapshots, 0);
        expect(errors, isEmpty);
      } finally {
        await session.stop().timeout(const Duration(seconds: 5));
      }
    },
    skip:
        Platform.environment['GBC_LIBRARY'] == null ||
        Platform.environment['GBC_ROM'] == null,
  );
  test(
    'native ROM produces nonuniform RGBA pixels that Flutter can decode',
    () async {
      final done = Completer<Uint8List>();
      var frames = 0;
      late EmulatorSession session;
      session = EmulatorSession(
        (bytes) {
          frames++;
          final words = bytes.buffer.asUint32List();
          if (frames > 20 && words.any((pixel) => pixel != words.first)) {
            if (!done.isCompleted) done.complete(bytes);
          } else if (frames >= 300) {
            done.completeError(
              StateError('No nonuniform video after 300 snapshots'),
            );
          } else {
            session.next();
          }
        },
        (error) {
          if (!done.isCompleted) done.completeError(StateError(error));
        },
      );
      try {
        await session.start(
          Platform.environment['GBC_LIBRARY']!,
          Platform.environment['GBC_ROM']!,
        );
        session.next();
        final bytes = await done.future.timeout(const Duration(seconds: 30));
        final decoded = Completer<ui.Image>();
        ui.decodeImageFromPixels(
          bytes,
          160,
          144,
          ui.PixelFormat.rgba8888,
          decoded.complete,
        );
        final image = await decoded.future;
        expect(image.width, 160);
        expect(image.height, 144);
        final png = await image.toByteData(format: ui.ImageByteFormat.png);
        final output = Platform.environment['GBC_FRAME_OUTPUT'];
        if (output != null) {
          File(output).writeAsBytesSync(png!.buffer.asUint8List());
        }
        image.dispose();
      } finally {
        await session.stop();
      }
    },
    skip:
        Platform.environment['GBC_LIBRARY'] == null ||
        Platform.environment['GBC_ROM'] == null,
  );
}
