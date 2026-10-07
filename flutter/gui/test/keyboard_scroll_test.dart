import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:gui/keyboard_scroll_area.dart';

void main() {
  testWidgets('page keys and endpoints scroll games but not while editing', (
    tester,
  ) async {
    final controller = ScrollController();
    await tester.pumpWidget(
      MaterialApp(
        home: Scaffold(
          body: Column(
            children: [
              const TextField(),
              Expanded(
                child: KeyboardScrollArea(
                  controller: controller,
                  child: ListView.builder(
                    controller: controller,
                    itemExtent: 100,
                    itemCount: 50,
                    itemBuilder: (_, i) => Text('Game $i'),
                  ),
                ),
              ),
            ],
          ),
        ),
      ),
    );
    await tester.pumpAndSettle();
    await tester.sendKeyEvent(LogicalKeyboardKey.pageDown);
    await tester.pumpAndSettle();
    expect(
      controller.offset,
      closeTo(controller.position.viewportDimension * 0.9, 1),
    );
    await tester.sendKeyEvent(LogicalKeyboardKey.end);
    await tester.pumpAndSettle();
    expect(controller.offset, controller.position.maxScrollExtent);
    await tester.sendKeyEvent(LogicalKeyboardKey.pageUp);
    await tester.pumpAndSettle();
    expect(controller.offset, lessThan(controller.position.maxScrollExtent));
    await tester.sendKeyEvent(LogicalKeyboardKey.home);
    await tester.pumpAndSettle();
    expect(controller.offset, 0);
    await tester.tap(find.byType(TextField));
    await tester.sendKeyEvent(LogicalKeyboardKey.end);
    await tester.pumpAndSettle();
    expect(controller.offset, 0);
    await tester.pumpWidget(const SizedBox());
    controller.dispose();
  });
}
