import 'package:flutter_test/flutter_test.dart';
import 'package:gbcemulator/gbcemulator.dart';
import 'package:gui/game_input.dart';

void main() {
  test('touch release preserves keyboard and controller holds', () {
    final events = <(Button, bool)>[];
    final input = GameInput((button, pressed) => events.add((button, pressed)));
    input.touch(Button.a, true);
    input.keyboard(Button.a, true);
    input.touch(Button.a, false);
    expect(events, [(Button.a, true)]);
    input.keyboard(Button.a, false);
    expect(events.last, (Button.a, false));
    input.controllers(1 << Button.left.index);
    input.touch(Button.left, true);
    input.controllers(0);
    expect(events.last, (Button.left, true));
    input.release();
    expect(events.last, (Button.left, false));
  });
  test('keyboard and controller releases preserve the other source', () {
    final events = <(Button, bool)>[];
    final input = GameInput((button, pressed) => events.add((button, pressed)));
    input.keyboard(Button.a, true);
    input.controllers(1 << Button.a.index);
    input.keyboard(Button.a, false);
    expect(events, [(Button.a, true)]);
    input.controllers(0);
    expect(events.last, (Button.a, false));
    input.keyboard(Button.b, true);
    input.controllers(1 << Button.b.index);
    input.controllers(0);
    expect(events.last, (Button.b, true));
    input.keyboard(Button.b, false);
    expect(events.last, (Button.b, false));
  });

  test(
    'focus loss releases all sources and resumed snapshots restore input',
    () {
      final events = <(Button, bool)>[];
      final input = GameInput(
        (button, pressed) => events.add((button, pressed)),
      );
      input.keyboard(Button.a, true);
      input.controllers(1 << Button.left.index);
      input.release();
      expect(events, [
        (Button.a, true),
        (Button.left, true),
        (Button.left, false),
        (Button.a, false),
      ]);
      input.controllers(1 << Button.left.index);
      expect(events.last, (Button.left, true));
      input.release();
      input.release();
      expect(events.length, 6);
    },
  );
}
