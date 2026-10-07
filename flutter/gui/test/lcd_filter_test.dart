import 'dart:ui' as ui;

import 'package:flutter_test/flutter_test.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();
  test('Authentic GBC Fast renders notches, brightness, black and bypass', () async {
    final program = await ui.FragmentProgram.fromAsset('shaders/rgb_lcd.frag');
    final shader = program.fragmentShader();
    Future<ui.Image> solid(ui.Color color) async {
      final recorder = ui.PictureRecorder();
      ui.Canvas(recorder).drawColor(color, ui.BlendMode.src);
      final picture = recorder.endRecording();
      final image = await picture.toImage(4, 4);
      picture.dispose();
      return image;
    }

    final gray = await solid(const ui.Color(0xff808080));
    final black = await solid(const ui.Color(0xff000000));
    shader
      ..setFloat(0, 48)
      ..setFloat(1, 48)
      ..setFloat(2, 4)
      ..setFloat(3, 4)
      ..setFloat(4, 1);
    Future<List<int>> render(
      ui.Image source,
      double strength,
      double brightness,
      double blur,
    ) async {
      shader
        ..setFloat(5, strength)
        ..setFloat(6, brightness)
        ..setFloat(7, blur)
        ..setImageSampler(0, source);
      final recorder = ui.PictureRecorder();
      ui.Canvas(recorder).drawRect(
        const ui.Rect.fromLTWH(0, 0, 48, 48),
        ui.Paint()..shader = shader,
      );
      final picture = recorder.endRecording();
      final image = await picture.toImage(48, 48);
      picture.dispose();
      final data = (await image.toByteData())!.buffer.asUint8List().toList();
      image.dispose();
      for (var i = 3; i < data.length; i += 4) {
        expect(data[i], 255);
      }
      return data;
    }

    try {
      final sharp = await render(gray, 1, 0, 0);
      int green(int x, int y) => sharp[(y * 48 + x) * 4 + 1];
      // At 12x, upstream green aperture spans x=4.224..7.776. Its bottom-left
      // notch removes x<5.604 for y>9.468; these samples lie inside the notch,
      // above it, and beside it. This checks actual upstream geometry.
      expect(green(4, 10), 0);
      expect(green(4, 5), greaterThan(0));
      expect(green(6, 10), closeTo(128, 1));
      expect(
        green(6, 5),
        closeTo(128, 1),
        reason: 'Squared input and sqrt output preserve gray',
      );
      final bright = await render(gray, 1, 0.6, 0.3);
      int sum(List<int> pixels) => pixels
          .asMap()
          .entries
          .where((e) => e.key % 4 != 3)
          .fold(0, (s, e) => s + e.value);
      expect(sum(bright), greaterThan(sum(sharp)));
      final bypass = await render(gray, 0, 0.6, 0.3);
      for (var i = 0; i < bypass.length; i++) {
        expect(bypass[i], i % 4 == 3 ? 255 : 128);
      }
      final dark = await render(black, 1, 0.6, 0.3);
      expect(sum(dark), 0);
    } finally {
      shader.dispose();
      gray.dispose();
      black.dispose();
    }
  });
}
