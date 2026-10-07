import 'dart:convert';
import 'dart:io';

/// Per-folder snapshots use relative ROM paths; device folder locations stay local.
class PortableSettings {
  static File copyFile(String root) => File('$root/.gbcemulator/settings.json');

  static String? relative(String root, String path) {
    String normalize(String value) =>
        value.replaceAll('\\', '/').replaceFirst(RegExp(r'/+$'), '');
    final base = normalize(Directory(root).absolute.path);
    final full = normalize(File(path).absolute.path);
    final matches = Platform.isWindows
        ? full.toLowerCase().startsWith('${base.toLowerCase()}/')
        : full.startsWith('$base/');
    return matches ? full.substring(base.length + 1) : null;
  }

  static DateTime date(dynamic value) =>
      DateTime.tryParse(value?.toString() ?? '')?.toUtc() ??
      DateTime.fromMillisecondsSinceEpoch(0, isUtc: true);

  static String nextTimestamp(dynamic previous) {
    final now = DateTime.now().toUtc();
    final old = date(previous);
    return (now.isAfter(old) ? now : old.add(const Duration(microseconds: 1)))
        .toIso8601String();
  }

  static void write(File file, Map<String, dynamic> data) {
    file.parent.createSync(recursive: true);
    final temporary = File('${file.path}.tmp');
    temporary.writeAsStringSync(
      const JsonEncoder.withIndent('  ').convert(data),
      flush: true,
    );
    temporary.renameSync(file.path);
  }

  static Map<String, dynamic> snapshot(
    Map<String, dynamic> data,
    String root,
    String revision,
  ) {
    Map<String, dynamic> entries(String field) => {
      for (final entry in (data[field] as Map? ?? {}).entries)
        if (relative(root, entry.key as String) case final String path)
          path: entry.value,
    };
    return {
      'version': 1,
      'kind': 'gbcemulator_rom_folder',
      'last_modified': revision,
      'playtime_seconds': entries('playtime_seconds'),
      'last_played': entries('last_played'),
    };
  }

  static Map<String, dynamic> read(File file) {
    final data = jsonDecode(file.readAsStringSync()) as Map<String, dynamic>;
    if (data['version'] != 1 ||
        data['kind'] != 'gbcemulator_rom_folder' ||
        DateTime.tryParse(data['last_modified']?.toString() ?? '') == null) {
      throw const FormatException('Invalid shared settings snapshot');
    }
    for (final field in ['playtime_seconds', 'last_played']) {
      final values = data[field];
      if (values is! Map) throw FormatException('Invalid $field');
      for (final entry in values.entries) {
        final path = entry.key as String;
        if (path.isEmpty ||
            path.contains('\\') ||
            path.contains(':') ||
            path
                .split('/')
                .any((part) => part.isEmpty || part == '.' || part == '..')) {
          throw const FormatException('Invalid relative ROM path');
        }
        if (field == 'playtime_seconds' &&
            (entry.value is! int || (entry.value as int) < 0)) {
          throw const FormatException('Invalid playtime');
        }
        if (field == 'last_played' &&
            DateTime.tryParse(entry.value.toString()) == null) {
          throw const FormatException('Invalid last-played timestamp');
        }
      }
    }
    return data;
  }

  static void import(
    Map<String, dynamic> local,
    Map<String, dynamic> remote,
    String root,
  ) {
    for (final field in ['playtime_seconds', 'last_played']) {
      final entries = Map<String, dynamic>.from(local[field] as Map? ?? {});
      entries.removeWhere((path, _) => relative(root, path) != null);
      for (final entry in (remote[field] as Map).entries) {
        entries[File('$root/${entry.key}').absolute.path] = entry.value;
      }
      local[field] = entries;
    }
  }
}
