import 'dart:convert';
import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:gui/folder_settings.dart';

void main() {
  test(
    'playtime accumulates without duplicate checkpoints and preserves folders',
    () async {
      final directory = await Directory.systemTemp.createTemp('gbc-playtime-');
      try {
        final file = File('${directory.path}/settings.json');
        final store = FolderSettings(file: file);
        await store.save([RomFolder(directory.path)]);
        final rom = File('${directory.path}/game.gb').absolute.path;
        store.addPlaytime(rom, 20);
        store.addPlaytime(rom, 30);
        final tracker = PlaytimeTracker(store, rom)..start();
        await Future<void>.delayed(const Duration(milliseconds: 1100));
        tracker.checkpoint();
        tracker.stop();
        tracker.stop();
        await store.save([RomFolder(directory.path, enabled: false)]);
        final data = jsonDecode(await file.readAsString());
        expect(data['playtime_seconds'][rom], 51);
        expect((data['playtime_seconds'] as Map).length, 1);
        expect((await store.load()).single.enabled, false);
        final unplayed = PlaytimeTracker(
          store,
          '${directory.path}/unplayed.gb',
        );
        unplayed.stop();
        expect(
          jsonDecode(await file.readAsString())['playtime_seconds'],
          data['playtime_seconds'],
        );
      } finally {
        await directory.delete(recursive: true);
      }
    },
  );
  test(
    'settings round trip preserves enabled state, removal and empty list',
    () async {
      final directory = await Directory.systemTemp.createTemp('gbc-settings-');
      try {
        final store = FolderSettings(
          file: File('${directory.path}/config/settings.json'),
        );
        expect(await store.load(), isEmpty);
        await store.save([
          RomFolder('C:/Games/GB'),
          RomFolder('C:/Games/GBC', enabled: false),
        ]);
        final loaded = await store.load();
        expect(loaded.map((f) => f.path), ['C:/Games/GB', 'C:/Games/GBC']);
        expect(loaded.map((f) => f.enabled), [true, false]);
        await store.save([loaded.last]);
        expect((await store.load()).single.enabled, false);
        await store.save([]);
        expect(await store.load(), isEmpty);
      } finally {
        await directory.delete(recursive: true);
      }
    },
  );
}
