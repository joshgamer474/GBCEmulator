import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';

/// Bookmarks remain device-local; shared settings contain no access credentials.
class RomFolderAccess {
  static const _channel = MethodChannel('gbcemulator/rom_folder_access');
  static bool get _enabled =>
      !kIsWeb && defaultTargetPlatform == TargetPlatform.macOS;

  static Future<List<String>> restore() async {
    if (!_enabled) return [];
    return await _channel.invokeListMethod<String>('restore') ?? [];
  }

  static Future<void> remember(String path) async {
    if (_enabled) await _channel.invokeMethod<void>('remember', path);
  }
}
