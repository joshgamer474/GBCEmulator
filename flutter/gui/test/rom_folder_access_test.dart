import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:gui/rom_folder_access.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();
  const channel = MethodChannel('gbcemulator/rom_folder_access');
  final calls = <MethodCall>[];

  setUp(() {
    debugDefaultTargetPlatformOverride = TargetPlatform.macOS;
    calls.clear();
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger
        .setMockMethodCallHandler(channel, (call) async {
          calls.add(call);
          return call.method == 'restore' ? <String>['Reconnect drive'] : null;
        });
  });

  tearDown(() {
    debugDefaultTargetPlatformOverride = null;
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger
        .setMockMethodCallHandler(channel, null);
  });

  test(
    'restores permissions and remembers selected folders on macOS',
    () async {
      expect(await RomFolderAccess.restore(), ['Reconnect drive']);
      await RomFolderAccess.remember('/Volumes/ROMs');
      expect(calls.map((call) => call.method), ['restore', 'remember']);
      expect(calls.last.arguments, '/Volumes/ROMs');
    },
  );

  test(
    'other desktop platforms do not invoke macOS bookmark methods',
    () async {
      debugDefaultTargetPlatformOverride = TargetPlatform.windows;
      expect(await RomFolderAccess.restore(), isEmpty);
      await RomFolderAccess.remember('C:/ROMs');
      expect(calls, isEmpty);
    },
  );

  test('bookmark failures are reported to the caller', () async {
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger
        .setMockMethodCallHandler(channel, (_) async {
          throw PlatformException(code: 'bookmark_failed');
        });
    await expectLater(
      RomFolderAccess.remember('/Volumes/ROMs'),
      throwsA(isA<PlatformException>()),
    );
  });
}
