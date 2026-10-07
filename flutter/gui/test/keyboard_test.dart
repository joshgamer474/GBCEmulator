import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:gbcemulator/gbcemulator.dart';
import 'package:gui/game_keyboard.dart';

void main() {
  testWidgets('SDL keys support chords, repeats, releases and focus loss', (
    tester,
  ) async {
    final events = <(Button, bool)>[];
    await tester.pumpWidget(
      MaterialApp(
        home: Scaffold(
          body: Column(
            children: [
              GameKeyboard(
                enabled: true,
                onButton: (b, down) => events.add((b, down)),
                child: const Text('Game'),
              ),
              const TextField(),
            ],
          ),
        ),
      ),
    );
    await tester.tap(find.text('Game'));
    await tester.pump();
    final keys = [
      LogicalKeyboardKey.keyW,
      LogicalKeyboardKey.keyA,
      LogicalKeyboardKey.keyS,
      LogicalKeyboardKey.keyD,
      LogicalKeyboardKey.keyZ,
      LogicalKeyboardKey.keyX,
      LogicalKeyboardKey.keyM,
      LogicalKeyboardKey.keyN,
    ];
    final buttons = [
      Button.up,
      Button.left,
      Button.down,
      Button.right,
      Button.a,
      Button.b,
      Button.start,
      Button.select,
    ];
    for (var i = 0; i < keys.length; i++) {
      await tester.sendKeyDownEvent(keys[i]);
      await tester.sendKeyRepeatEvent(keys[i]);
      await tester.sendKeyUpEvent(keys[i]);
      expect(events.sublist(i * 2), [(buttons[i], true), (buttons[i], false)]);
    }
    events.clear();
    await tester.sendKeyDownEvent(LogicalKeyboardKey.keyW);
    await tester.sendKeyDownEvent(LogicalKeyboardKey.keyZ);
    await tester.tap(find.byType(TextField));
    await tester.pump();
    expect(events, [
      (Button.up, true),
      (Button.a, true),
      (Button.up, false),
      (Button.a, false),
    ]);
    await tester.sendKeyUpEvent(LogicalKeyboardKey.keyW);
    await tester.sendKeyUpEvent(LogicalKeyboardKey.keyZ);
    expect(events.length, 4);
  });
}
