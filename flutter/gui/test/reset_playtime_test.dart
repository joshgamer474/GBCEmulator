import 'dart:convert';
import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:GBCEmulator/folder_settings.dart';
import 'package:GBCEmulator/portable_settings.dart';

void main() {
  test('reset persists, updates shared copy, and preserves other games', () async {
    final temp = Directory.systemTemp.createTempSync('gbc-reset-');
    try {
      final store = FolderSettings(file: File('${temp.path}/settings.json'));
      await store.save([RomFolder(temp.path, shareSettings: true)]);
      final rom = '${temp.path}/Game.gb';
      final other = '${temp.path}/Other.gb';
      store.addPlaytime(rom, 120);
      store.addPlaytime(other, 60);
      final copy = PortableSettings.copyFile(temp.path);
      final before = PortableSettings.read(copy)['last_modified'];

      store.resetPlaytime(rom);

      final reloaded = FolderSettings(file: store.file);
      expect(reloaded.playtimes().containsKey(folderKey(rom)), false);
      expect(reloaded.playtimes()[folderKey(other)], 60);
      expect(reloaded.recentlyPlayed().map(folderKey), [folderKey(other)]);
      final local = jsonDecode(store.file.readAsStringSync()) as Map;
      expect((local['last_played'] as Map).keys.map((p) => folderKey(p as String)),
          [folderKey(other)]);
      final shared = PortableSettings.read(copy);
      expect(shared['playtime_seconds'], {'Other.gb': 60});
      expect(shared['last_modified'], isNot(before));

      store.addPlaytime(rom, 5);
      expect(store.playtimes()[folderKey(rom)], 5);
    } finally {
      temp.deleteSync(recursive: true);
    }
  });
}
