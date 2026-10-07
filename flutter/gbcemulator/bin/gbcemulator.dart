import 'dart:io';

import 'package:gbcemulator/gbcemulator.dart';

void main(List<String> arguments) {
  List<String> arguments = [
    'C:\\Users\\childers\\Documents\\Git\\GBCEmulator\\build\\flutter-ffi\\Debug\\gbcemulator_ffi.dll',
    'C:\\Users\\childers\\Downloads\\Legend of Zelda, The - Oracle of Seasons (U) [C][!].zip',
  ];

  if (arguments.length != 2) {
    stderr.writeln(
      'Usage: dart run bin/gbcemulator.dart <native-library> <rom>',
    );
    exitCode = 64;
    return;
  }

  final String library = arguments[0];
  final String rom = arguments[1];
  final emulator = GBCEmulator.create(rom, library: openLibrary(library));
  try {
    emulator.run_next_instruction();
    print('Frame buffer: ${emulator.get_frame().length} RGBA bytes');
  } finally {
    emulator.destroy();
  }
}

