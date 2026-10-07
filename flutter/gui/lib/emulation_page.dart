import 'dart:async';
import 'dart:io';
import 'dart:ui' as ui;

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import 'emulator_session.dart';
import 'game_keyboard.dart';
import 'folder_settings.dart';
import 'controller_focus.dart';
import 'lcd_filter.dart';
import 'boxart_catalog.dart';
import 'boxart_image.dart';

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

class _EmulationPageState extends State<EmulationPage> {
  EmulatorSession? _session;
  late final PlaytimeTracker _playtime;
  late final AppLifecycleListener _lifecycle;
  Timer? _saveTimer;
  Future<void>? _stopped;
  ui.Image? _image;
  bool _lcdEnabled = true;
  double _lcdBrightness = 0.93;
  String _status = 'Loading ROM...';
  bool _playing = false;
  bool _leaving = false;
  bool _canPop = false;
  final _scaffoldKey = GlobalKey<ScaffoldState>();
  bool _drawerOpen = false;
  ControllerFocus? _controllerFocus;
  bool get _desktop =>
      Platform.isWindows || Platform.isLinux || Platform.isMacOS;

  @override
  void initState() {
    super.initState();
    _playtime = PlaytimeTracker(
      widget.settings ?? FolderSettings(),
      widget.rom,
    );
    _lifecycle = AppLifecycleListener(
      onExitRequested: () async {
        try {
          _playtime.checkpoint();
          await _shutdown();
          return ui.AppExitResponse.exit;
        } catch (error) {
          _saveError(error);
          return ui.AppExitResponse.cancel;
        }
      },
      onStateChange: (state) {
        if (!_desktop) {
          _session?.controllersEnabled = state == AppLifecycleState.resumed;
        }
        _checkpoint();
      },
    );
    unawaited(_start());
  }

  Future<void> _start() async {
    late final EmulatorSession session;
    session = EmulatorSession(
      (bytes) => _showFrame(session, bytes),
      (error) {
        if (mounted && !_leaving) {
          setState(() {
            _status = error;
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
        try {
          _playtime.start();
        } catch (error) {
          _saveError(error);
        }
        _saveTimer = Timer.periodic(
          const Duration(minutes: 1),
          (_) => _checkpoint(),
        );
        session.next();
      }
    } catch (error) {
      if (mounted && !_leaving) setState(() => _status = error.toString());
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
    try {
      _playtime.checkpoint();
    } catch (error) {
      _saveError(error);
    }
  }

  Future<void> _shutdown() {
    _controllerFocus?.dispose();
    _controllerFocus = null;
    _saveTimer?.cancel();
    try {
      _playtime.stop();
    } catch (error) {
      _saveError(error);
    }
    final session = _session;
    _session = null;
    return _stopped ??= session?.stop() ?? Future<void>.value();
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
    ui.decodeImageFromPixels(bytes, 160, 144, ui.PixelFormat.rgba8888, (image) {
      if (!mounted || _leaving || !identical(_session, session)) {
        image.dispose();
        return;
      }
      final previous = _image;
      setState(() => _image = image);
      WidgetsBinding.instance.addPostFrameCallback((_) => previous?.dispose());
      session.next();
    });
  }

  @override
  void dispose() {
    _lifecycle.dispose();
    unawaited(_shutdown());
    _image?.dispose();
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
                    style: Theme.of(context).textTheme.titleLarge?.copyWith(fontWeight: FontWeight.bold),
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
                    title: const Text('RGB LCD filter', style: TextStyle(fontWeight: FontWeight.bold)),
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
                  //Text(_status, maxLines: 3, overflow: TextOverflow.ellipsis),
                  //const SizedBox(height: 12),
                  Expanded(
                    child: GameKeyboard(
                      enabled: _playing && !_drawerOpen,
                      onButton: (button, pressed) =>
                          _session?.button(button, pressed),
                      child: Center(
                        child: AspectRatio(
                          aspectRatio: 160 / 144,
                          child: ColoredBox(
                            color: Colors.black,
                            child: _image == null
                                ? const Center(child: Text('Waiting for video'))
                                : LcdFilter(
                                    image: _image!,
                                    enabled: _lcdEnabled,
                                    brightness: _lcdBrightness,
                                  ),
                          ),
                        ),
                      ),
                    ),
                  ),
                ],
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
                      icon: Icon(
                        Icons.menu,
                        color: Colors.white,
                      ),
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
