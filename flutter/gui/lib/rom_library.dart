import 'dart:io';

String gameName(String path) => path.replaceAll('\\', '/').split('/').last;

enum GameRegion { all, usa, eu, jpn }

bool matchesRegion(String path, GameRegion region) {
  if (region == GameRegion.all) return true;
  final regions = <GameRegion>{};
  for (final group in RegExp(r'\(([^()]*)\)').allMatches(gameName(path))) {
    for (final tag in group.group(1)!.toLowerCase().split(RegExp(r'[,/\s]+'))) {
      if (tag == 'world') return true;
      switch (tag) {
        case 'u':
        case 'us':
        case 'usa':
          regions.add(GameRegion.usa);
        case 'e':
        case 'eu':
        case 'europe':
        case 'german':
        case 'germany':
        case 'france':
          regions.add(GameRegion.eu);
        case 'j':
        case 'jp':
        case 'jpn':
        case 'japan':
          regions.add(GameRegion.jpn);
        default:
          // GoodTools combined region tags, such as (UE) or (JUE).
          if (RegExp(r'^[uje]{2,3}$').hasMatch(tag)) {
            if (tag.contains('u')) regions.add(GameRegion.usa);
            if (tag.contains('e')) regions.add(GameRegion.eu);
            if (tag.contains('j')) regions.add(GameRegion.jpn);
          }
      }
    }
  }
  return regions.contains(region);
}

Future<List<String>> scanRoms(String folder) async {
  final games = <String>[];
  await for (final entry in Directory(
    folder,
  ).list(recursive: true, followLinks: false)) {
    if (entry is File &&
        RegExp(r'\.(gb|gbc|zip)$', caseSensitive: false).hasMatch(entry.path)) {
      games.add(entry.path);
    }
  }
  games.sort(
    (a, b) => gameName(a).toLowerCase().compareTo(gameName(b).toLowerCase()),
  );
  return games;
}
