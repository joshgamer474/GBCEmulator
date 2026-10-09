import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';

/// Bookmarks remain device-local; shared settings contain no access credentials.
class RomFolderAccess {
  static const _channel = MethodChannel('gbcemulator/rom_folder_access');
  static bool get _enabled =>
      !kIsWeb && (defaultTargetPlatform == TargetPlatform.macOS ||
        defaultTargetPlatform == TargetPlatform.iOS);

  static Future<String?> pickIOSFolder() =>
      _channel.invokeMethod<String>('pick');

  static Future<String> iosSettingsPath() async {
    final path = await _channel.invokeMethod<String>('settingsPath');
    if (path == null || path.isEmpty) {
      throw StateError('iOS did not provide a settings directory');
    }
    return path;
  }

  static Future<List<String>> restore() async {
    if (!_enabled) return [];
    return await _channel.invokeListMethod<String>('restore') ?? [];
  }

  static Future<void> remember(String path) async {
    if (_enabled) await _channel.invokeMethod<void>('remember', path);
  }
}
