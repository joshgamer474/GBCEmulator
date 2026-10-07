import 'dart:convert';

import 'package:flutter/services.dart';

import 'rom_library.dart';

String shortRomName(String name) {
  name = gameName(name)
      .replaceFirst(RegExp(r'\.(png|gbc|gb|zip)$', caseSensitive: false), '');
  while (true) {
    final cleaned = name.replaceAll(RegExp(r'\([^()]*\)|\[[^\[\]]*\]'), '');
    if (cleaned == name) break;
    name = cleaned;
  }
  return name.replaceAll(RegExp(r'\s+'), ' ').trim();
}

String romMatchKey(String name) =>
    shortRomName(name)
        .toLowerCase()
        .replaceAll('é', 'e')
        .replaceAll(RegExp(r'[^a-z0-9]+'), ' ')
        .trim();

class BoxArt {
  const BoxArt(this.system, this.filename, this.shortName, this.url);
  final String system;
  final String filename;
  final String shortName;
  final String url;
}

class BoxArtCatalog {
  final Map<String, BoxArt?> _cache = {};
  final Map<String, List<BoxArt>> _index = {};
  final Map<String, List<BoxArt>> _subtitleIndex = {};
  BoxArtCatalog.fromJson(String json) {
    final data = jsonDecode(json) as Map<String, dynamic>;
    if (data['schema_version'] != 1) {
      throw const FormatException('Unsupported box-art catalog');
    }
    for (final row in data['entries'] as List) {
      final art = BoxArt(
        row['system'] as String,
        row['filename'] as String,
        row['short_name'] as String,
        row['image_url'] as String,
      );
      (_index[row['match_key'] as String] ??= []).add(art);
      final parts = shortRomName(art.filename).split(' - ');
      if (parts.length > 2) {
        final key = romMatchKey(parts.take(parts.length - 1).join(' - '));
        (_subtitleIndex[key] ??= []).add(art);
      }
    }
  }

  static Future<BoxArtCatalog> load() async => BoxArtCatalog.fromJson(
    await rootBundle.loadString('assets/boxart_catalog.json'),
  );

  BoxArt? match(String rom) {
    if (_cache.containsKey(rom)) return _cache[rom];
    return _cache[rom] = _match(rom);
  }

  /// Token Dice similarity, not a calibrated probability. Require most words
  /// from both titles, and never infer a different numbered sequel.
  static double titleSimilarity(String first, String second) {
    Set<String> tokens(String title) => romMatchKey(title)
        .split(' ')
        .where(
          (word) => !{'', 's', 'the', 'a', 'an', 'of', 'and'}.contains(word),
        )
        .toSet();
    final a = tokens(first);
    final b = tokens(second);
    if (a.length < 3 || b.length < 3) return 0;
    final numbers = RegExp(r'^\d+$');
    final an = a.where(numbers.hasMatch).toSet();
    final bn = b.where(numbers.hasMatch).toSet();
    if (an.length != bn.length || !an.containsAll(bn)) return 0;
    final overlap = a.intersection(b).length;
    if (overlap / a.length < 0.8 || overlap / b.length < 0.65) return 0;
    return 2 * overlap / (a.length + b.length);
  }

  BoxArt? _match(String rom) {
    final key = romMatchKey(rom);
    final filename = gameName(rom).toLowerCase();
    final stem = filename.replaceFirst(RegExp(r'\.(gbc|gb|zip)$'), '');
    final system =
        filename.endsWith('.gbc') || RegExp(r'\[c\]').hasMatch(filename)
        ? 'gbc'
        : filename.endsWith('.gb')
        ? 'gb'
        : null;
    List<BoxArt> forSystem(List<BoxArt>? entries) => (entries ?? [])
        .where((art) => system == null || art.system == system)
        .toList();
    var filtered = forSystem(_index[key]);
    if (filtered.isEmpty) {
      filtered = forSystem(_subtitleIndex[key]);
      // Regional duplicates are fine; different subtitles on the same console
      // are ambiguous, so don't guess between those games.
      final titles = <String, Set<String>>{};
      for (final art in filtered) {
        (titles[art.system] ??= {}).add(romMatchKey(art.filename));
      }
      filtered = filtered
          .where((art) => titles[art.system]!.length == 1)
          .toList();
    }
    if (filtered.isEmpty) {
      // Rank distinct normalized titles, not regional cover duplicates.
      final matches = <(double, List<BoxArt>)>[];
      for (final entry in _index.entries) {
        final candidates = forSystem(entry.value);
        if (candidates.isEmpty) continue;
        final similarity = titleSimilarity(key, entry.key);
        if (similarity > 0) matches.add((similarity, candidates));
      }
      matches.sort((a, b) => b.$1.compareTo(a.$1));
      if (matches.isNotEmpty &&
          matches.first.$1 >= 0.8 &&
          (matches.length == 1 || matches.first.$1 - matches[1].$1 >= 0.08)) {
        filtered = matches.first.$2;
      }
    }
    // Don't substitute a different console's version when the extension is known.
    if (filtered.isEmpty) return null;
    int score(BoxArt art) {
      final name = art.filename.toLowerCase();
      if (name.replaceFirst(RegExp(r'\.png$'), '') == stem) return 1000;
      final us = RegExp(r'\((usa|u|us|ue)([,)]|$)').hasMatch(filename);
      final europe = RegExp(r'\((europe|e|eu)([,)]|$)').hasMatch(filename);
      final japan = RegExp(r'\((japan|j|jp)([,)]|$)').hasMatch(filename);
      var result = 0;
      if ((us && name.contains('(usa')) ||
          (europe && name.contains('(europe')) ||
          (japan && name.contains('(japan'))) {
        result += 100;
      }
      if (name.contains('(usa')) result += 30;
      if (name.contains('(europe')) result += 20;
      if (name.contains('(japan')) result += 10;
      if (name.contains('(proto') || name.contains('(beta')) result -= 50;
      return result;
    }

    final ranked = [...filtered]
      ..sort((a, b) {
        final difference = score(b) - score(a);
        return difference != 0 ? difference : a.url.compareTo(b.url);
      });
    return ranked.first;
  }
}
