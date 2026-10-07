import 'dart:io';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:gui/main.dart';
import 'package:gui/emulation_page.dart';
import 'package:gui/rom_library.dart';

void main() {
  late Directory folder;
  setUp(() {
    folder = Directory.systemTemp.createTempSync('gbc-library-test-');
    File('${folder.path}/Alpha.GBC').writeAsBytesSync([]);
    Directory('${folder.path}/nested').createSync();
    File('${folder.path}/nested/Beta.gb').writeAsBytesSync([]);
    File('${folder.path}/Gamma.zip').writeAsBytesSync([]);
    File('${folder.path}/ignore.sav').writeAsBytesSync([]);
  });
  tearDown(() => folder.deleteSync(recursive: true));

  test(
    'scan includes supported extensions and subfolders, sorted by name',
    () async {
      expect((await scanRoms(folder.path)).map(gameName), [
        'Alpha.GBC',
        'Beta.gb',
        'Gamma.zip',
      ]);
    },
  );

  testWidgets(
    'library scans and Escape returns from automatically opened game',
    (tester) async {
      await tester.runAsync(() async {
        await tester.pumpWidget(
          MyApp(settingsFile: File('${folder.path}/settings.json')),
        );
        await Future<void>.delayed(const Duration(milliseconds: 100));
      });
      await tester.pumpAndSettle();
      await tester.pumpAndSettle();
      expect(find.text('Game Library'), findsOneWidget);
      await tester.enterText(
        find.byType(TextField).first,
        '${folder.path}/missing.dll',
      );
      await tester.enterText(find.byType(TextField).last, folder.path);
      await tester.runAsync(() async {
        await tester.tap(find.text('Add path'));
        await Future<void>.delayed(const Duration(milliseconds: 150));
      });
      await tester.pumpAndSettle();
      expect(find.text('Alpha.GBC'), findsOneWidget);
      await tester.runAsync(() async {
        await tester.tap(find.text('Alpha.GBC'));
        await tester.pump();
        await Future<void>.delayed(const Duration(milliseconds: 200));
      });
      await tester.pumpAndSettle();
      expect(find.byType(EmulationPage), findsOneWidget);
      await tester.runAsync(() async {
        await tester.sendKeyEvent(LogicalKeyboardKey.escape);
        await Future<void>.delayed(const Duration(milliseconds: 100));
      });
      await tester.pumpAndSettle();
      expect(find.text('Game Library'), findsOneWidget);
      expect(find.text('Beta.gb'), findsOneWidget);
      expect(find.byType(EmulationPage), findsNothing);
    },
  );
}


