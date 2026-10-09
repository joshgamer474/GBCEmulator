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

  test('iOS uses the native folder picker and restores saved access', () async {
    debugDefaultTargetPlatformOverride = TargetPlatform.iOS;
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger
        .setMockMethodCallHandler(channel, (call) async {
          calls.add(call);
          return call.method == 'pick' ? '/Documents/ROMs' : <String>[];
        });
    expect(await RomFolderAccess.restore(), isEmpty);
    expect(await RomFolderAccess.pickIOSFolder(), '/Documents/ROMs');
    expect(calls.map((call) => call.method), ['restore', 'pick']);
  });

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

  test('iOS settings use the native sandbox directory', () async {
    debugDefaultTargetPlatformOverride = TargetPlatform.iOS;
    const path = '/app/Library/Application Support/GBCEmulator/settings.json';
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger
        .setMockMethodCallHandler(channel, (call) async {
          calls.add(call);
          return path;
        });
    expect(await RomFolderAccess.iosSettingsPath(), path);
    expect(calls.single.method, 'settingsPath');
  });

  test('missing iOS settings path does not fall back outside the sandbox', () async {
    await expectLater(RomFolderAccess.iosSettingsPath(), throwsStateError);
  });
}
