import 'dart:async';
import 'dart:io';
import 'dart:ui' as ui;

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter/scheduler.dart';

import 'emulator_session.dart';
import 'game_keyboard.dart';
import 'folder_settings.dart';
import 'controller_focus.dart';
import 'lcd_filter.dart';
import 'boxart_catalog.dart';
import 'boxart_image.dart';
import 'mobile_controls.dart';
import 'frame_timing_stats.dart';
import 'video_frame_queue.dart';

// Enable when troubleshooting native-to-Flutter frame delivery.
const bool _showFrameDelivery = false;

class EmulationPage extends StatefulWidget {
  const EmulationPage({
    super.key,
    required this.library,
    required this.rom,
    this.settings,
    this.boxArtUrl,
  });
  final String library;
  final String rom;
  final FolderSettings? settings;
  final String? boxArtUrl;
  @override
  State<EmulationPage> createState() => _EmulationPageState();
}

class _EmulationPageState extends State<EmulationPage>
    with SingleTickerProviderStateMixin {
  EmulatorSession? _session;
  late final PlaytimeTracker _playtime;
  late final AppLifecycleListener _lifecycle;
  late final Ticker _videoTicker;

  void _updateVideoTicker() {
    final lifecycle = WidgetsBinding.instance.lifecycleState;
    final active =
        _playing &&
        !_leaving &&
        (lifecycle == null || lifecycle == AppLifecycleState.resumed);
    if (active && !_videoTicker.isActive) {
      _videoTicker.start();
    } else if (!active && _videoTicker.isActive) {
      _videoTicker.stop();
    }
  }

  Timer? _saveTimer;
  Timer? _fpsTimer;
  final _fpsClock = Stopwatch();
  int _receivedFrames = 0;
  int _decodedFrames = 0;
  int _presentedFrames = 0;
  int _vsyncTicks = 0;
  int _decodedBursts = 0;
  int _decodedGaps = 0;
  int? _lastDecodedAt;
  String _fps = 'Collecting frame statistics…';
  final _frameTimings = FrameTimingStats();
  String _timingSummary = 'Flutter timings: collecting…';

  void _recordTimings(List<ui.FrameTiming> timings) {
    if (_playing && !_leaving) _frameTimings.add(timings);
  }

  Future<void>? _stopped;
  ui.Image? _image;
  final _video = ValueNotifier<ui.Image?>(null);
  final _videoFrames = VideoFrameQueue<ui.Image>((image) => image.dispose());
  ui.Image? _paintedImage;
  bool _imagePresentationScheduled = false;
  bool _lcdEnabled = true;
  double _lcdBrightness = 0.93;
  String _status = 'Loading ROM...';
  String? _startupError;
  bool _playing = false;
  bool _leaving = false;
  bool _canPop = false;
  final _scaffoldKey = GlobalKey<ScaffoldState>();
  bool _drawerOpen = false;
  ControllerFocus? _controllerFocus;
  bool get _desktop =>
      Platform.isWindows || Platform.isLinux || Platform.isMacOS;
  bool get _mobile => Platform.isIOS || Platform.isAndroid;

  @override
  void initState() {
    super.initState();
    // Keep vsync scheduling active during playback. Images still update only
    // the video subtree; this does not rebuild the page or change native speed.
    _videoTicker = createTicker((_) {
      _vsyncTicks++;
      final image = _videoFrames.take();
      if (image != null) _presentFrame(image);
    });
    WidgetsBinding.instance.addTimingsCallback(_recordTimings);
    _playtime = PlaytimeTracker(
      widget.settings ?? FolderSettings(),
      widget.rom,
    );
    _lifecycle = AppLifecycleListener(
      onExitRequested: () async {
        try {
          await _playtime.checkpointAsync();
          await _shutdown();
          return ui.AppExitResponse.exit;
        } catch (error) {
          _saveError(error);
          return ui.AppExitResponse.cancel;
        }
      },
      onStateChange: (state) {
        _updateVideoTicker();
        if (!_desktop) {
          _session?.controllersEnabled = state == AppLifecycleState.resumed;
        }
        _checkpoint();
      },
    );
    // Let the route paint its controls before native controller initialization
    // and the first image upload begin.
    WidgetsBinding.instance.addPostFrameCallback((_) {
      // Cocoa controller setup can process native callbacks. Run it after
      // Flutter has left its frame callback, rather than re-entering a draw.
      Timer.run(() {
        if (mounted && !_leaving) unawaited(_start());
      });
    });
  }

  Future<void> _start() async {
    late final EmulatorSession session;
    session = EmulatorSession(
      (bytes) => _showFrame(session, bytes),
      (error) {
        if (mounted && !_leaving) {
          setState(() {
            _status = error;
            _startupError = error;
            _playing = false;
          });
          unawaited(_shutdown());
        }
      },
      onControllerError: (error) {
        WidgetsBinding.instance.addPostFrameCallback((_) {
          if (mounted && !_leaving) {
            ScaffoldMessenger.of(context)
                .showSnackBar(SnackBar(content: Text(error)));
          }
        });
      },
    );
    session.controllersEnabled = false;
    _session = session;
    try {
      // Start first so shutdown always has a worker completion to await.
      final started = session.start(widget.library, widget.rom);
      if (_desktop) {
        _controllerFocus = ControllerFocus((focused) {
          if (!_leaving && identical(_session, session)) {
            session.controllersEnabled = focused;
          }
        });
        // Attach the startup error handler before awaiting either operation.
        await Future.wait([started, _controllerFocus!.start()]);
      } else {
        session.controllersEnabled =
            WidgetsBinding.instance.lifecycleState == AppLifecycleState.resumed;
        await started;
      }
      if (mounted && !_leaving && identical(_session, session)) {
        setState(() {
          _playing = true;
          _status =
              'W A S D: Move   Z/X: A/B   M/N: Start/Select   Esc: Library';
        });
        _updateVideoTicker();
        try {
          unawaited(_playtime.startAsync().catchError(_saveError));
        } catch (error) {
          _saveError(error);
        }
        _saveTimer = Timer.periodic(
          const Duration(minutes: 1),
          (_) => _checkpoint(),
        );
        session.next();
        var previous = (
          session.nativeFrames,
          _receivedFrames,
          _decodedFrames,
          _presentedFrames,
          _vsyncTicks,
          _decodedBursts,
          _decodedGaps,
        );
        var previousTime = 0.0;
        _fpsClock.start();
        _fpsTimer = Timer.periodic(const Duration(seconds: 2), (_) {
          if (!mounted || _leaving) return;
          final now = _fpsClock.elapsedMicroseconds / 1000000;
          final seconds = now - previousTime;
          final counts = (
            session.nativeFrames,
            _receivedFrames,
            _decodedFrames,
            _presentedFrames,
            _vsyncTicks,
            _decodedBursts,
            _decodedGaps,
          );
          String rate(int count, int old) =>
              ((count - old) / seconds).toStringAsFixed(1);
          final native = session.nativeFps?.toStringAsFixed(1) ?? 'N/A';
          setState(() {
            _fps =
                'Native: $native fps\nReceived: ${rate(counts.$2, previous.$2)} fps\nDecoded: ${rate(counts.$3, previous.$3)} fps\nPresented: ${rate(counts.$4, previous.$4)} fps\nObserved vsync: ${rate(counts.$5, previous.$5)} fps\nDecode intervals (${seconds.toStringAsFixed(1)} s sample):\n<8 ms: ${counts.$6 - previous.$6}, >25 ms: ${counts.$7 - previous.$7}';
            _timingSummary =
                '${_frameTimings.summary}\nReported display: ${View.of(context).display.refreshRate.toStringAsFixed(1)} Hz';
          });
          previous = counts;
          previousTime = now;
        });
      }
    } catch (error) {
      if (mounted && !_leaving) {
        setState(() {
          _status = error.toString();
          _startupError ??= _status;
        });
      }
      await _shutdown();
    }
  }

  void _saveError(Object error) {
    if (mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Could not save playtime: $error')),
      );
    }
  }

  void _checkpoint() {
    unawaited(_playtime.checkpointAsync().catchError(_saveError));
  }

  Future<void> _shutdown() {
    if (_stopped != null) return _stopped!;
    _videoTicker.stop();
    _videoFrames.clear();
    _controllerFocus?.dispose();
    _controllerFocus = null;
    _saveTimer?.cancel();
    _fpsTimer?.cancel();
    _fpsClock.stop();
    final saved = _playtime.stopAsync().catchError(_saveError);
    final session = _session;
    _session = null;
    return _stopped = Future.wait<void>([
      saved,
      session?.stop() ?? Future<void>.value(),
    ]).then((_) {});
  }

  Future<void> _leave() async {
    if (_leaving) return;
    setState(() {
      _leaving = true;
      _playing = false;
      _status = 'Stopping...';
    });
    await _shutdown();
    if (!mounted) return;
    setState(() => _canPop = true);
    // Allow PopScope to rebuild before popping, after native teardown completes.
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (mounted) Navigator.of(context).pop();
    });
  }

  void _showFrame(EmulatorSession session, Uint8List bytes) {
    if (_leaving || !identical(_session, session)) return;
    _receivedFrames++;
    ui.decodeImageFromPixels(bytes, 160, 144, ui.PixelFormat.rgba8888, (image) {
      if (!mounted || _leaving || !identical(_session, session)) {
        image.dispose();
        return;
      }
      _decodedFrames++;
      final decodedAt = _fpsClock.elapsedMicroseconds;
      if (_lastDecodedAt != null) {
        final interval = decodedAt - _lastDecodedAt!;
        if (interval < 8000) _decodedBursts++;
        if (interval > 25000) _decodedGaps++;
      }
      _lastDecodedAt = decodedAt;
      _videoFrames.add(image);
      session.next();
    });
  }

  void _presentFrame(ui.Image image) {
    final previous = _image;
    // An image replaced before a Flutter paint can be discarded immediately.
    // Keep the last painted image alive until its replacement is painted.
    if (!identical(previous, _paintedImage)) previous?.dispose();
    _image = image;
    _video.value = image;
    if (!_imagePresentationScheduled) {
      _imagePresentationScheduled = true;
      WidgetsBinding.instance.addPostFrameCallback((_) {
        _imagePresentationScheduled = false;
        if (!mounted) return;
        final oldPainted = _paintedImage;
        _paintedImage = _image;
        if (!identical(oldPainted, _paintedImage)) _presentedFrames++;
        if (!identical(oldPainted, _paintedImage)) oldPainted?.dispose();
      });
    }
  }

  @override
  void dispose() {
    WidgetsBinding.instance.removeTimingsCallback(_recordTimings);
    _lifecycle.dispose();
    unawaited(_shutdown());
    _videoTicker.dispose();
    _image?.dispose();
    if (!identical(_paintedImage, _image)) _paintedImage?.dispose();
    _video.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) => PopScope<void>(
    canPop: _canPop,
    onPopInvokedWithResult: (didPop, _) {
      if (!didPop) unawaited(_leave());
    },
    child: CallbackShortcuts(
      bindings: {
        const SingleActivator(LogicalKeyboardKey.keyL): () {
          setState(() => _lcdEnabled = !_lcdEnabled);
          ScaffoldMessenger.of(context)
            ..hideCurrentSnackBar()
            ..showSnackBar(
              SnackBar(
                content: Text('RGB LCD filter: ${_lcdEnabled ? 'On' : 'Off'}'),
                duration: const Duration(seconds: 1),
              ),
            );
        },
        const SingleActivator(LogicalKeyboardKey.escape): () {
          if (_drawerOpen) {
            _scaffoldKey.currentState?.closeDrawer();
          } else {
            unawaited(_leave());
          }
        },
      },
      child: Focus(
        autofocus: true,
        child: Scaffold(
          key: _scaffoldKey,
          backgroundColor: Colors.black,
          onDrawerChanged: (open) => setState(() => _drawerOpen = open),
          drawer: Drawer(
            width: 320,
            child: SafeArea(
              child: ListView(
                padding: const EdgeInsets.all(16),
                children: [
                  Align(
                    alignment: Alignment.centerRight,
                    child: IconButton(
                      tooltip: 'Close settings',
                      onPressed: () => _scaffoldKey.currentState?.closeDrawer(),
                      icon: const Icon(Icons.close),
                    ),
                  ),
                  SizedBox(
                    height: 240,
                    child: BoxArtImage(
                      url: widget.boxArtUrl,
                      semanticLabel: '${shortRomName(widget.rom)} box art',
                      iconSize: 80,
                    ),
                  ),
                  const SizedBox(height: 16),
                  Text(
                    shortRomName(widget.rom),
                    textAlign: TextAlign.center,
                    style: Theme.of(context).textTheme.titleLarge
                        ?.copyWith(fontWeight: FontWeight.bold),
                  ),
                  const SizedBox(height: 8),
                  SelectableText(
                    File(widget.rom).absolute.path,
                    textAlign: TextAlign.center,
                    style: Theme.of(context).textTheme.bodySmall?.copyWith(
                      color: Theme.of(context).colorScheme.onSurfaceVariant,
                    ),
                  ),
                  const SizedBox(height: 12),
                  SwitchListTile(
                    contentPadding: EdgeInsets.zero,
                    title: const Text(
                      'RGB LCD filter',
                      style: TextStyle(fontWeight: FontWeight.bold),
                    ),
                    value: _lcdEnabled,
                    onChanged: (enabled) =>
                        setState(() => _lcdEnabled = enabled),
                  ),
                  Text(
                    'LCD brightness: ${(_lcdBrightness * 100).round()}%',
                    style: Theme.of(context).textTheme.bodyMedium,
                  ),
                  Slider(
                    value: _lcdBrightness,
                    min: 0.1,
                    max: 1.0,
                    divisions: 90,
                    label: '${(_lcdBrightness * 100).round()}%',
                    semanticFormatterCallback: (value) =>
                        'LCD brightness ${(value * 100).round()} percent',
                    onChanged: (value) =>
                        setState(() => _lcdBrightness = value),
                  ),
                  const Divider(),
                  if (_showFrameDelivery)
                    ListTile(
                      contentPadding: EdgeInsets.zero,
                      title: const Text('Frame delivery'),
                      subtitle: Text(
                        '$_fps\n\n$_timingSummary\nPresented counts Flutter frame submissions, not physical display refreshes. Native N/A means the native library needs rebuilding.',
                      ),
                    ),
                  ListTile(
                    contentPadding: EdgeInsets.zero,
                    leading: const Icon(Icons.backspace),
                    title: const Text('Exit'),
                    enabled: !_leaving,
                    onTap: () {
                      _scaffoldKey.currentState?.closeDrawer();
                      unawaited(_leave());
                    },
                  ),
                ],
              ),
            ),
          ),
          body: Stack(
            fit: StackFit.expand,
            children: [
              Column(
                children: [
                  if (!_playing && _startupError == null)
                    Padding(
                      padding: const EdgeInsets.fromLTRB(60, 16, 16, 12),
                      child: Text(
                        _status,
                        maxLines: 3,
                        overflow: TextOverflow.ellipsis,
                      ),
                    ),
                  Expanded(
                    child: GameKeyboard(
                      enabled: _playing && !_drawerOpen,
                      onButton: (button, pressed) =>
                          _session?.button(button, pressed),
                      child: Padding(
                        padding: !_mobile
                            ? EdgeInsets.zero
                            : MediaQuery.orientationOf(context) ==
                                  Orientation.portrait
                            ? EdgeInsets.only(
                                bottom:
                                    212 + MediaQuery.paddingOf(context).bottom,
                              )
                            : const EdgeInsets.fromLTRB(140, 0, 140, 64),
                        child: Center(
                          child: AspectRatio(
                            aspectRatio: 160 / 144,
                            child: ColoredBox(
                              color: Colors.black,
                              child: RepaintBoundary(
                                child: ValueListenableBuilder<ui.Image?>(
                                  valueListenable: _video,
                                  builder: (context, image, _) => image == null
                                      ? Center(
                                          child: SingleChildScrollView(
                                            padding: const EdgeInsets.all(16),
                                            child: Text(
                                              _startupError ??
                                                  'Waiting for video',
                                              textAlign: TextAlign.center,
                                            ),
                                          ),
                                        )
                                      : LcdFilter(
                                          image: image,
                                          enabled: _lcdEnabled,
                                          brightness: _lcdBrightness,
                                        ),
                                ),
                              ),
                            ),
                          ),
                        ),
                      ),
                    ),
                  ),
                ],
              ),
              if (_mobile)
                Positioned(
                  left: 0,
                  right: 0,
                  bottom: 0,
                  child: MobileControls(
                    enabled: _playing && !_drawerOpen && !_leaving,
                    onButton: (button, pressed) =>
                        _session?.touchButton(button, pressed),
                  ),
                ),
              Positioned(
                top: 12,
                left: 12,
                child: SafeArea(
                  child: Material(
                    color: Colors.transparent,
                    borderRadius: BorderRadius.circular(12),
                    clipBehavior: Clip.antiAlias,
                    child: IconButton(
                      tooltip: 'Emulation settings',
                      onPressed: _leaving
                          ? null
                          : () => _scaffoldKey.currentState?.openDrawer(),
                      icon: Icon(Icons.menu, color: Colors.white),
                    ),
                  ),
                ),
              ),
            ],
          ),
        ),
      ),
    ),
  );
}
