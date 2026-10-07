import 'dart:convert';
import 'dart:io';

import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:gui/main.dart';
import 'package:gui/folder_settings.dart';

void main() {
  testWidgets(
    'recent games appear only with playtime and hide while searching',
    (tester) async {
      final dir = Directory.systemTemp.createTempSync('gbc-recent-');
      addTearDown(() => dir.deleteSync(recursive: true));
      final rom = File('${dir.path}/Sample.gb')..writeAsBytesSync([]);
      final settings = FolderSettings(file: File('${dir.path}/settings.json'));
      await tester.runAsync(() async {
        await settings.save([RomFolder(dir.path)]);
        settings.addPlaytime(rom.path, 20);
        await tester.pumpWidget(MyApp(settingsFile: settings.file));
        await Future<void>.delayed(const Duration(milliseconds: 200));
      });
      await tester.pumpAndSettle();
      expect(find.text('Recently Played'), findsOneWidget);
      final search = find.widgetWithText(TextField, 'Search games');
      await tester.enterText(search, 'sample');
      await tester.pumpAndSettle();
      expect(find.text('Recently Played'), findsNothing);
      await tester.enterText(search, '');
      await tester.pumpAndSettle();
      expect(find.text('Recently Played'), findsOneWidget);
      await tester.pumpWidget(const SizedBox());
      await tester.runAsync(() async {
        await settings.file.writeAsString(
          jsonEncode({
            'version': 1,
            'rom_folders': [
              {'path': dir.path, 'enabled': true},
            ],
            'playtime_seconds': {rom.path: 0},
          }),
        );
        await tester.pumpWidget(MyApp(settingsFile: settings.file));
        await Future<void>.delayed(const Duration(milliseconds: 200));
      });
      await tester.pumpAndSettle();
      expect(find.text('Recently Played'), findsNothing);
      await tester.pumpWidget(const SizedBox());
    },
  );

  test(
    'dated games sort newest first and legacy played entries are retained',
    () async {
      final dir = Directory.systemTemp.createTempSync('gbc-recent-order-');
      try {
        final store = FolderSettings(file: File('${dir.path}/settings.json'));
        store.file.writeAsStringSync(
          jsonEncode({
            'version': 1,
            'rom_folders': [],
            'playtime_seconds': {
              'old.gb': 10,
              'new.gb': 20,
              'legacy.gb': 5,
              'zero.gb': 0,
            },
            'last_played': {
              'old.gb': '2026-01-01T00:00:00Z',
              'new.gb': '2026-02-01T00:00:00Z',
            },
          }),
        );
        expect(store.recentlyPlayed(), ['new.gb', 'old.gb', 'legacy.gb']);
      } finally {
        dir.deleteSync(recursive: true);
      }
    },
  );
}
