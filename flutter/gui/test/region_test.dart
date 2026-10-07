import 'package:flutter_test/flutter_test.dart';
import 'package:gui/rom_library.dart';

void main() {
  test('region aliases, combined tags and World', () {
    for (final tag in ['U', 'USA']) {
      expect(matchesRegion('Game ($tag).gb', GameRegion.usa), isTrue);
      expect(matchesRegion('Game ($tag).gb', GameRegion.eu), isFalse);
    }
    for (final tag in ['E', 'Europe', 'German', 'France', 'Germany']) {
      expect(matchesRegion('Game ($tag).zip', GameRegion.eu), isTrue);
    }
    for (final tag in ['J', 'Japan']) {
      expect(matchesRegion('Game ($tag).gbc', GameRegion.jpn), isTrue);
    }
    for (final tag in ['USA, Europe', 'UE']) {
      expect(matchesRegion('Game ($tag).zip', GameRegion.usa), isTrue);
      expect(matchesRegion('Game ($tag).zip', GameRegion.eu), isTrue);
      expect(matchesRegion('Game ($tag).zip', GameRegion.jpn), isFalse);
    }
    for (final region in GameRegion.values) {
      expect(matchesRegion('Game (World).gb', region), isTrue);
    }
    expect(matchesRegion('Game (En,Fr,De).gb', GameRegion.eu), isFalse);
    expect(matchesRegion('C:/USA/Game.gb', GameRegion.usa), isFalse);
    expect(matchesRegion('Game.gb', GameRegion.all), isTrue);
    expect(matchesRegion('Game (Unknown).gb', GameRegion.usa), isFalse);
  });
}
