import 'dart:convert';
import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:gui/boxart_catalog.dart';

void main() {
  final json = File('assets/boxart_catalog.json').readAsStringSync();
  final catalog = BoxArtCatalog.fromJson(json);
  test('all scraped keys match Dart normalization and URLs are HTTPS PNGs', () {
    final rows = (jsonDecode(json) as Map<String, dynamic>)['entries'] as List;
    expect(rows.length, greaterThan(3000));
    final urls = <String>{};
    for (final row in rows) {
      expect(romMatchKey(row['filename'] as String), row['match_key']);
      expect(shortRomName(row['filename'] as String), row['short_name']);
      final uri = Uri.parse(row['image_url'] as String);
      expect(uri.scheme, 'https');
      expect(uri.host, 'thumbnails.libretro.com');
      expect(uri.path.endsWith('.png'), isTrue);
      expect(urls.add(uri.toString()), isTrue);
    }
  });
  test('Yellow matches when the catalog has an extra subtitle', () {
    final art = catalog.match('Pokemon - Yellow Version (USA).gb');
    expect(
      art?.shortName,
      'Pokemon - Yellow Version - Special Pikachu Edition',
    );
    expect(art?.system, 'gb');
    expect(catalog.match('Pokemon - Yellow Version (UE) [!].zip'), isNotNull);
    expect(catalog.match('Pokemon.gb'), isNull);
    expect(catalog.match('Legend of Zelda, The.gb'), isNull);
  });
  test(
    'scored fallback matches publisher prefixes without guessing sequels',
    () {
      final art = catalog.match('Power Rangers - Lightspeed Rescue (USA).gbc');
      expect(art, isNotNull);
      expect(art!.shortName, contains('Lightspeed Rescue'));
      expect(art.shortName, contains('Saban'));
      expect(art.system, 'gbc');
      expect(
        BoxArtCatalog.titleSimilarity(
          'Power Rangers - Lightspeed Rescue',
          "Saban's Power Rangers - Lightspeed Rescue",
        ),
        closeTo(0.8889, 0.001),
      );
      expect(
        BoxArtCatalog.titleSimilarity('Super Mario Land', 'Super Mario Land 2'),
        0,
      );
      expect(catalog.match('Power Rangers.gbc'), isNull);
    },
  );
  test('GoodTools tags, paths, case and Pokemon variants match', () {
    final art = catalog.match(
      r'C:\ROMs\Pokemon - Silver Version (UE) [C][!].zip',
    );
    expect(art, isNotNull);
    expect(art!.system, 'gbc');
    expect(art.filename.toLowerCase(), contains('silver version'));
    expect(catalog.match('TETRIS (World).GB')?.system, 'gb');
    expect(
      catalog
          .match('Legend of Zelda, The - Oracle of Seasons (U) [C][!].zip')
          ?.system,
      'gbc',
    );
    expect(catalog.match('My completely unknown homebrew.gb'), isNull);
  });
  test('console and exact region win; unknown console is not substituted', () {
    final fixture = BoxArtCatalog.fromJson(
      jsonEncode({
        'schema_version': 1,
        'entries': [
          for (final row in [
            ('gb', 'Same (USA).png'),
            ('gbc', 'Same (Japan).png'),
            ('gbc', 'Same (USA).png'),
          ])
            {
              'system': row.$1,
              'filename': row.$2,
              'short_name': 'Same',
              'match_key': 'same',
              'image_url': row.$1 + row.$2,
            },
          {
            'system': 'gbc',
            'filename': 'Color Only.png',
            'short_name': 'Color Only',
            'match_key': 'color only',
            'image_url': 'color',
          },
        ],
      }),
    );
    expect(fixture.match('Same.gb')?.system, 'gb');
    expect(fixture.match('Same (Japan).gbc')?.filename, 'Same (Japan).png');
    expect(fixture.match('Color Only.gb'), isNull);
  });
}
