import 'dart:convert';
import 'dart:io';

import 'portable_settings.dart';

class RomFolder {
  RomFolder(this.path, {this.enabled = true, this.shareSettings = false});
  final String path;
  bool enabled;
  bool shareSettings;
}

String formatPlaytime(int seconds) {
  final minutes = seconds < 0 ? 0 : seconds ~/ 60;
  if (minutes < 60) return 'Played $minutes min';
  return 'Played ${minutes ~/ 60} hours ${minutes % 60} min';
}

String folderKey(String path) {
  final value = Directory(path).absolute.path
      .replaceAll('\\', '/')
      .replaceFirst(RegExp(r'/+$'), '');
  return Platform.isWindows ? value.toLowerCase() : value;
}

class FolderSettings {
  FolderSettings({File? file})
    : file =
          file ??
          File(
            '${Platform.environment['APPDATA'] ?? Platform.environment['XDG_CONFIG_HOME'] ?? '${Platform.environment['HOME'] ?? Directory.current.path}/.config'}/GBCEmulator/settings.json',
          );
  final File file;
  final Map<String, String> syncWarnings = {};

  Future<List<RomFolder>> load() async {
    if (!await file.exists()) return [];
    final data = _read();
    if (data['version'] != 1) {
      throw const FormatException('Unsupported settings version');
    }
    final seen = <String>{};
    return (data['rom_folders'] as List)
        .map(
          (row) => RomFolder(
            row['path'] as String,
            enabled: row['enabled'] as bool,
            shareSettings: row['share_settings'] == true,
          ),
        )
        .where(
          (folder) =>
              folder.path.trim().isNotEmpty && seen.add(folderKey(folder.path)),
        )
        .toList();
  }

  Map<String, dynamic> _read() {
    if (!file.existsSync()) return {'version': 1, 'rom_folders': []};
    final data = jsonDecode(file.readAsStringSync()) as Map<String, dynamic>;
    if (data['version'] != 1) {
      throw const FormatException('Unsupported settings version');
    }
    _sync(data);
    return data;
  }

  void _sync(Map<String, dynamic> data) {
    final shared = {
      for (final row in data['rom_folders'] as List? ?? [])
        if (row['share_settings'] == true) folderKey(row['path'] as String),
    };
    syncWarnings.removeWhere((key, _) => !shared.contains(key));
    final revisions = Map<String, dynamic>.from(
      data['folder_modified'] as Map? ?? {},
    );
    for (final row in data['rom_folders'] as List? ?? []) {
      if (row['share_settings'] != true) continue;
      final root = row['path'] as String;
      final key = folderKey(root);
      try {
        if (!Directory(root).existsSync()) {
          throw FileSystemException('Shared ROM folder is unavailable', root);
        }
        final copy = PortableSettings.copyFile(root);
        final remote = copy.existsSync() ? PortableSettings.read(copy) : null;
        var baseline = DateTime.fromMillisecondsSinceEpoch(0, isUtc: true);
        for (final entry in (data['last_played'] as Map? ?? {}).entries) {
          final date = PortableSettings.date(entry.value);
          if (PortableSettings.relative(root, entry.key as String) != null &&
              date.isAfter(baseline)) {
            baseline = date;
          }
        }
        final revision = revisions[key] ?? baseline.toIso8601String();
        if (remote != null &&
            PortableSettings.date(remote['last_modified'])
                .isAfter(PortableSettings.date(revision))) {
          PortableSettings.import(data, remote, root);
          revisions[key] = remote['last_modified'];
          if (PortableSettings.date(remote['last_modified'])
              .isAfter(PortableSettings.date(data['last_modified']))) {
            data['last_modified'] = remote['last_modified'];
          }
        } else {
          revisions[key] = remote == null && revisions[key] == null
              ? PortableSettings.nextTimestamp(revision)
              : revision;
        }
        data['folder_modified'] = revisions;
        // Commit locally first. A failed mirror write must not cause a playtime
        // checkpoint retry to count the same elapsed seconds twice.
        PortableSettings.write(file, data);
        final snapshot = PortableSettings.snapshot(
          data,
          root,
          revisions[key] as String,
        );
        if (remote == null || jsonEncode(remote) != jsonEncode(snapshot)) {
          PortableSettings.write(copy, snapshot);
        }
        syncWarnings.remove(key);
      } catch (error) {
        syncWarnings[key] = 'Could not sync $root: $error';
      }
    }
  }

  void _write(Map<String, dynamic> data) {
    data['last_modified'] = PortableSettings.nextTimestamp(
      data['last_modified'],
    );
    PortableSettings.write(file, data);
    _sync(data);
  }

  // Synchronous read-modify-write keeps settings updates serialized on the UI
  // isolate and lets the final checkpoint complete during window teardown.
  Future<void> save(List<RomFolder> folders) async {
    final data = _read();
    data['rom_folders'] = [
      for (final folder in folders)
        {
          'path': folder.path,
          'enabled': folder.enabled,
          'share_settings': folder.shareSettings,
        },
    ];
    // Import existing folder copies before assigning a new local timestamp.
    _sync(data);
    _write(data);
  }

  /// Older settings have no timestamps; keep those after dated entries.
  Map<String, int> playtimes() {
    final times = Map<String, dynamic>.from(
      _read()['playtime_seconds'] as Map? ?? {},
    );
    return {
      for (final entry in times.entries)
        if (entry.value is num)
          folderKey(entry.key): (entry.value as num).toInt(),
    };
  }

  List<String> recentlyPlayed() {
    final data = _read();
    final times = Map<String, dynamic>.from(
      data['playtime_seconds'] as Map? ?? {},
    );
    final dates = Map<String, dynamic>.from(data['last_played'] as Map? ?? {});
    final paths = times.keys
        .where((path) => times[path] is num && (times[path] as num) > 0)
        .toList();
    DateTime date(String path) =>
        DateTime.tryParse(dates[path]?.toString() ?? '') ??
        DateTime.fromMillisecondsSinceEpoch(0, isUtc: true);
    paths.sort((a, b) {
      final order = date(b).compareTo(date(a));
      return order != 0 ? order : a.toLowerCase().compareTo(b.toLowerCase());
    });
    return paths;
  }

  void addPlaytime(String rom, int seconds) {
    if (seconds < 0) throw ArgumentError.value(seconds, 'seconds');
    final data = _read();
    final times = Map<String, dynamic>.from(
      data['playtime_seconds'] as Map? ?? {},
    );
    final path = File(rom).absolute.path;
    final key = times.keys.firstWhere(
      (key) => folderKey(key) == folderKey(path),
      orElse: () => path,
    );
    times[key] = ((times[key] as num?)?.toInt() ?? 0) + seconds;
    data['playtime_seconds'] = times;
    final dates = Map<String, dynamic>.from(data['last_played'] as Map? ?? {});
    dates[key] = DateTime.now().toUtc().toIso8601String();
    data['last_played'] = dates;
    _markPlaytimeModified(data, path);
    _write(data);
  }

  void resetPlaytime(String rom) {
    final data = _read();
    final path = File(rom).absolute.path;
    for (final field in ['playtime_seconds', 'last_played']) {
      final entries = Map<String, dynamic>.from(data[field] as Map? ?? {});
      entries.removeWhere((key, _) => folderKey(key) == folderKey(path));
      data[field] = entries;
    }
    _markPlaytimeModified(data, path);
    _write(data);
  }

  void _markPlaytimeModified(Map<String, dynamic> data, String path) {
    final revisions = Map<String, dynamic>.from(
      data['folder_modified'] as Map? ?? {},
    );
    for (final row in data['rom_folders'] as List? ?? []) {
      final root = row['path'] as String;
      if (PortableSettings.relative(root, path) != null) {
        revisions[folderKey(root)] = PortableSettings.nextTimestamp(
          revisions[folderKey(root)] ?? data['last_modified'],
        );
      }
    }
    data['folder_modified'] = revisions;
  }
}

/// Saves only the elapsed whole seconds not already checkpointed this session.
class PlaytimeTracker {
  PlaytimeTracker(this.settings, this.rom);
  final FolderSettings settings;
  final String rom;
  final Stopwatch _clock = Stopwatch();
  int _savedSeconds = 0;
  bool _started = false;

  void start() {
    if (_started) return;
    _started = true;
    _clock.start();
    settings.addPlaytime(rom, 0);
  }

  void checkpoint() {
    if (!_started) return;
    final seconds = _clock.elapsed.inSeconds;
    final delta = seconds - _savedSeconds;
    if (delta == 0) return;
    settings.addPlaytime(rom, delta);
    _savedSeconds = seconds;
  }

  void stop() {
    _clock.stop();
    checkpoint();
  }
}
