import 'dart:async';

import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:gui/controller_focus.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();
  const channel = MethodChannel('window_manager');
  final messenger =
      TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger;
  tearDown(() => messenger.setMockMethodCallHandler(channel, null));

  test(
    'initial native focus enables input without a resumed lifecycle event',
    () async {
      messenger.setMockMethodCallHandler(channel, (call) async {
        expect(call.method, 'isFocused');
        return true;
      });
      final states = <bool>[];
      final focus = ControllerFocus(states.add);
      await focus.start();
      expect(states, [false, true]);
      focus.onWindowBlur();
      focus.onWindowFocus();
      expect(states, [false, true, false, true]);
      focus.dispose();
    },
  );

  test(
    'a late initial focus query cannot undo a blur event or disposal',
    () async {
      final reply = Completer<bool>();
      messenger.setMockMethodCallHandler(channel, (_) => reply.future);
      final states = <bool>[];
      final focus = ControllerFocus(states.add);
      final started = focus.start();
      focus.onWindowBlur();
      reply.complete(true);
      await started;
      expect(states, [false, false]);
      focus.dispose();
      focus.onWindowFocus();
      expect(states, [false, false]);
    },
  );
}
