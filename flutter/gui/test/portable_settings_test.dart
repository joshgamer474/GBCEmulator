import 'dart:convert';
import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:gui/folder_settings.dart';
import 'package:gui/portable_settings.dart';

void main() {
  late Directory temp;
  setUp(() => temp = Directory.systemTemp.createTempSync('gbc-share-'));
  tearDown(() => temp.deleteSync(recursive: true));

  test(
    'multiple folders stay independent and disabling sharing stops updates',
    () async {
      final a = Directory('${temp.path}/a')..createSync();
      final b = Directory('${temp.path}/b')..createSync();
      final store = FolderSettings(
        file: File('${temp.path}/config/settings.json'),
      );
      await store.save([
        RomFolder(a.path, shareSettings: true),
        RomFolder(b.path, shareSettings: true),
      ]);
      store.addPlaytime('${a.path}/Same.gb', 10);
      store.addPlaytime('${b.path}/Same.gb', 20);
      expect(
        PortableSettings.read(
          PortableSettings.copyFile(a.path),
        )['playtime_seconds'],
        {'Same.gb': 10},
      );
      expect(
        PortableSettings.read(
          PortableSettings.copyFile(b.path),
        )['playtime_seconds'],
        {'Same.gb': 20},
      );
      await store.save([
        RomFolder(a.path),
        RomFolder(b.path, shareSettings: true),
      ]);
      final before = PortableSettings.copyFile(a.path).readAsStringSync();
      store.addPlaytime('${a.path}/Same.gb', 5);
      expect(store.playtimes()[folderKey('${a.path}/Same.gb')], 15);
      expect(PortableSettings.copyFile(a.path).readAsStringSync(), before);
    },
  );
  test(
    'optional snapshots rebase paths and synchronize newest playtime',
    () async {
      final a = Directory('${temp.path}/desktop')..createSync();
      final b = Directory('${temp.path}/laptop')..createSync();
      final first = FolderSettings(
        file: File('${temp.path}/config-a/settings.json'),
      );
      final second = FolderSettings(
        file: File('${temp.path}/config-b/settings.json'),
      );
      await first.save([RomFolder(a.path)]);
      first.addPlaytime('${a.path}/Game.gb', 60);
      expect(PortableSettings.copyFile(a.path).existsSync(), false);
      await first.save([RomFolder(a.path, shareSettings: true)]);
      final copyA = PortableSettings.copyFile(a.path);
      final copyB = PortableSettings.copyFile(b.path);
      copyB.parent.createSync();
      copyA.copySync(copyB.path);
      await second.save([RomFolder(b.path, shareSettings: true)]);
      expect(second.playtimes()[folderKey('${b.path}/Game.gb')], 60);
      second.addPlaytime('${b.path}/Game.gb', 30);
      copyB.copySync(copyA.path);
      expect(first.playtimes()[folderKey('${a.path}/Game.gb')], 90);
      final portable = jsonDecode(copyA.readAsStringSync());
      expect(portable['playtime_seconds'], {'Game.gb': 90});
      expect(DateTime.tryParse(portable['last_modified']), isNotNull);
      expect((await first.load()).single.path, a.path);
      expect((await first.load()).single.shareSettings, true);
      // A stale incoming snapshot is replaced by the newer local state.
      portable['last_modified'] = '2000-01-01T00:00:00Z';
      portable['playtime_seconds']['Game.gb'] = 1;
      copyA.writeAsStringSync(jsonEncode(portable));
      expect(first.playtimes()[folderKey('${a.path}/Game.gb')], 90);
      expect(
        jsonDecode(copyA.readAsStringSync())['playtime_seconds']['Game.gb'],
        90,
      );
    },
  );

  test(
    'unavailable copies never prevent local checkpoints or create ROM folders',
    () async {
      final root = Directory('${temp.path}/roms')..createSync();
      final store = FolderSettings(
        file: File('${temp.path}/config/settings.json'),
      );
      await store.save([RomFolder(root.path, shareSettings: true)]);
      root.deleteSync(recursive: true);
      store.addPlaytime('${root.path}/Game.gb', 15);
      expect(store.playtimes()[folderKey('${root.path}/Game.gb')], 15);
      expect(root.existsSync(), false);
      expect(store.syncWarnings, isNotEmpty);
    },
  );

  test(
    'invalid shared paths are rejected without overwriting the bad copy',
    () async {
      final root = Directory('${temp.path}/roms')..createSync();
      final copy = PortableSettings.copyFile(root.path);
      copy.parent.createSync();
      final invalid = jsonEncode({
        'version': 1,
        'kind': 'gbcemulator_rom_folder',
        'last_modified': '2099-01-01T00:00:00Z',
        'playtime_seconds': {'../escape.gb': 123},
        'last_played': {},
      });
      copy.writeAsStringSync(invalid);
      final store = FolderSettings(
        file: File('${temp.path}/config/settings.json'),
      );
      await store.save([RomFolder(root.path, shareSettings: true)]);
      expect(store.playtimes(), isEmpty);
      expect(store.syncWarnings, isNotEmpty);
      expect(copy.readAsStringSync(), invalid);
    },
  );
}
