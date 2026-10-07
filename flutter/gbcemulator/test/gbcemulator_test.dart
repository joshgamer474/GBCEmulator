import 'dart:async';
import 'dart:ffi';
import 'dart:io';

import 'package:ffi/ffi.dart';
import 'package:gbcemulator/gbcemulator.dart';
import 'package:test/test.dart';

void main() {
  final libraryPath = Platform.environment['GBC_LIBRARY'];
  final romPath = Platform.environment['GBC_ROM'];
  test('frame callback is one-shot and can be rearmed', () async {
    final emulator = GBCEmulator.create(
      romPath!,
      library: openLibrary(libraryPath!),
    );
    var count = 0;
    var received = Completer<void>();
    emulator.set_frame_callback(() {
      count++;
      if (!received.isCompleted) received.complete();
    });
    try {
      emulator.request_frame();
      emulator.request_frame(); // Coalesce requests before the native thread starts.
      emulator.run();
      await received.future.timeout(const Duration(seconds: 5));
      expect(emulator.get_frame().length, GBCEmulator.frame_bytes);
      await Future<void>.delayed(const Duration(milliseconds: 100));
      expect(count, 1);
      received = Completer<void>();
      emulator.request_frame();
      await received.future.timeout(const Duration(seconds: 5));
      expect(count, 2);
      emulator.request_frame(); // Teardown also handles an outstanding request.
    } finally {
      emulator.destroy();
    }
    final stoppedCount = count;
    await Future<void>.delayed(const Duration(milliseconds: 50));
    expect(count, stoppedCount);
  }, skip: libraryPath == null || romPath == null);

  test(
    'native exports and argument failures',
    () {
      final bindings = GBCBindings(openLibrary(libraryPath!));
      expect(bindings.create(nullptr), nullptr);
      bindings.destroy(nullptr);
      expect(bindings.set_joypad_button(nullptr, 0), -1);
      expect(bindings.release_joypad_button(nullptr, 0), -1);
      expect(bindings.run_next_instruction(nullptr), -1);
      expect(bindings.run(nullptr), -1);
      final buffer = calloc<Uint8>(GBCEmulator.frame_bytes);
      try {
        expect(
          bindings.get_frame(nullptr, buffer, GBCEmulator.frame_bytes),
          -1,
        );
      } finally {
        calloc.free(buffer);
      }
      expect(
        () => GBCEmulator.create('', library: openLibrary(libraryPath)),
        throwsArgumentError,
      );
    },
    skip: libraryPath == null ? 'Set GBC_LIBRARY to the built library' : false,
  );

  test(
    'ROM lifecycle, input, stepping and independent frame snapshots',
    () {
      final emulator = GBCEmulator.create(
        romPath!,
        library: openLibrary(libraryPath!),
      );
      try {
        emulator.set_joypad_button(Button.a);
        emulator.run_next_instruction();
        emulator.release_joypad_button(Button.a);
        emulator.run();
        emulator.run(); // Idempotent: no second native thread.
        expect(emulator.run_next_instruction, throwsStateError);
        final first = emulator.get_frame();
        expect(first.length, GBCEmulator.frame_bytes);
        expect(first[3], 255);
        first[3] = 0;
        expect(emulator.get_frame()[3], 255);
        emulator.destroy();
        expect(first[3], 0);
        expect(emulator.get_frame, throwsStateError);
        expect(emulator.run_next_instruction, throwsStateError);
        expect(() => emulator.set_joypad_button(Button.a), throwsStateError);
        expect(
          () => emulator.release_joypad_button(Button.a),
          throwsStateError,
        );
      } finally {
        emulator.destroy();
      }
    },
    skip: libraryPath == null || romPath == null
        ? 'Set GBC_LIBRARY and GBC_ROM for the ROM integration test'
        : false,
  );
}
