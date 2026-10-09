import 'dart:async';
import 'dart:io';

import 'package:file_selector/file_selector.dart';
import 'package:flutter/material.dart';
import 'package:flutter/rendering.dart';
import 'package:flutter/services.dart';
import 'package:package_info_plus/package_info_plus.dart';
import 'package:window_manager/window_manager.dart';

import 'emulation_page.dart';
import 'rom_library.dart';
import 'boxart_catalog.dart';
import 'folder_settings.dart';
import 'keyboard_scroll_area.dart';
import 'native_library_path.dart';
import 'rom_folder_access.dart';
import 'boxart_image.dart';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  if (Platform.isWindows || Platform.isLinux || Platform.isMacOS) {
    await windowManager.ensureInitialized();
  }
  runApp(const MyApp());
}

bool _togglingFullscreen = false;

Future<void> _toggleFullscreen() async {
  if (_togglingFullscreen ||
      !(Platform.isWindows || Platform.isLinux || Platform.isMacOS)) {
    return;
  }
  _togglingFullscreen = true;
  try {
    await windowManager.setFullScreen(!await windowManager.isFullScreen());
  } catch (error, stack) {
    FlutterError.reportError(
      FlutterErrorDetails(
        exception: error,
        stack: stack,
        context: ErrorDescription('while toggling fullscreen'),
      ),
    );
  } finally {
    _togglingFullscreen = false;
  }
}

class MyApp extends StatelessWidget {
  const MyApp({super.key, this.settingsFile});
  final File? settingsFile;
  @override
  Widget build(BuildContext context) => MaterialApp(
    title: 'GBCEmulator',
    builder: (context, child) => CallbackShortcuts(
      bindings: {
        const SingleActivator(
          LogicalKeyboardKey.f11,
          includeRepeats: false,
        ): () =>
            unawaited(_toggleFullscreen()),
      },
      child: child!,
    ),
    theme: ThemeData(
      appBarTheme: const AppBarTheme(centerTitle: false),
      colorScheme: ColorScheme.fromSeed(
        seedColor: Colors.indigo,
        brightness: Brightness.dark,
      ).copyWith(primary: Colors.indigoAccent, onPrimary: Colors.white),
    ),
    home: GameLibraryPage(settingsFile: settingsFile),
  );
}

class GameLibraryPage extends StatefulWidget {
  const GameLibraryPage({super.key, this.settingsFile});
  final File? settingsFile;
  @override
  State<GameLibraryPage> createState() => _GameLibraryPageState();
}

class _GameLibraryPageState extends State<GameLibraryPage> {
  final _packageInfo = PackageInfo.fromPlatform();
  final _folder = TextEditingController(
    text: const String.fromEnvironment('GBC_ROM_FOLDER'),
  );
  List<String> _games = [];
  List<String> _recent = [];
  Map<String, int> _playtimes = {};

  Future<void> _loadRecent() async {
    try {
      final (playtimes, recent) = await _settings.historyAsync();
      if (!mounted) return;
      setState(() {
        _recent = recent;
        _playtimes = playtimes;
        if (_settings.syncWarnings.isNotEmpty) {
          _error = _settings.syncWarnings.values.join('\n');
        }
      });
    } catch (error) {
      if (mounted) {
        setState(() => _error = 'Could not load recently played games: $error');
      }
    }
  }

  List<String> get _recentGames {
    final available = {
      for (final path in _filteredGames) folderKey(path): path,
    };
    return [
      for (final path in _recent)
        if (available.containsKey(folderKey(path))) available[folderKey(path)]!,
    ];
  }

  List<String> _filteredGames = [];
  final _search = TextEditingController();
  GameRegion _region = GameRegion.all;

  void _filterGames() {
    final words = _search.text.trim().toLowerCase().split(RegExp(r'\s+'));
    _filteredGames = _games.where((path) {
      final title = shortRomName(path).toLowerCase();
      return words.every(title.contains) && matchesRegion(path, _region);
    }).toList();
  }

  void _searchChanged(String _) {
    setState(_filterGames);
    if (_tileScroll.hasClients) _tileScroll.jumpTo(0);
  }

  List<RomFolder> _folders = [];
  late final FolderSettings _settings;
  bool _settingsReady = false;
  BoxArtCatalog? _covers;
  String? _coverError;
  final _tileScroll = ScrollController();
  final _gridKey = GlobalKey();

  bool _loading = false;
  Future<void>? _scanInProgress;
  bool _choosing = false;
  late final Future<void> _restored;
  bool _opening = false;
  String? _error;

  @override
  void initState() {
    super.initState();
    unawaited(_loadCovers());
    const rom = String.fromEnvironment('GBC_ROM');
    if (_folder.text.isEmpty && rom.isNotEmpty) {
      _folder.text = File(rom).parent.path;
    }
    _restored = _restore();
    unawaited(_restored);
  }

  Future<void> _loadCovers() async {
    try {
      final catalog = await BoxArtCatalog.load();
      if (mounted) setState(() => _covers = catalog);
    } catch (_) {
      if (mounted) {
        setState(
          () => _coverError =
              'Cover catalog unavailable. Games can still be played.',
        );
      }
    }
  }

  Future<void> _restore() async {
    try {
      final settingsFile =
          widget.settingsFile ??
          (Platform.isIOS
              ? File(await RomFolderAccess.iosSettingsPath())
              : null);
      _settings = FolderSettings(file: settingsFile);
      // Restore sandbox access before load(), which also synchronizes shared
      // folder settings, and before the first directory scan.
      final accessWarnings = await RomFolderAccess.restore();
      if (accessWarnings.isNotEmpty && mounted) {
        setState(() => _error = accessWarnings.join('\n'));
      }
      final folders = await _settings.load();
      if (!mounted) return;
      _folders = folders;
      if (!await _settings.file.exists() && _folder.text.isNotEmpty) {
        _folders.add(RomFolder(Directory(_folder.text.trim()).absolute.path));
        await _settings.save(_folders);
      }
    } catch (error) {
      if (mounted) setState(() => _error = 'Could not load settings: $error');
      return;
    }
    if (!mounted) return;
    setState(() => _settingsReady = true);
    await _scan();
  }

  Future<void> _scan() {
    if (_scanInProgress != null) return _scanInProgress!;
    final scan = _scanImpl().whenComplete(() => _scanInProgress = null);
    _scanInProgress = scan;
    return scan;
  }

  Future<void> _scanImpl() async {
    if (_loading || !_settingsReady) return;
    setState(() {
      _loading = true;
      _games = [];
      _filteredGames = [];
    });
    final games = <String, String>{};
    final errors = <String>[];
    for (final folder in _folders.where((entry) => entry.enabled)) {
      try {
        for (final game in await scanRoms(folder.path)) {
          games.putIfAbsent(folderKey(game), () => game);
        }
      } catch (error) {
        if ((Platform.isMacOS || Platform.isIOS) &&
            error is FileSystemException &&
            error.osError?.errorCode == 1) {
          errors.add(
            '${folder.path}: Access denied. Select this folder again with '
            'Add ROM Folder to restore permission.',
          );
        } else {
          errors.add('${folder.path}: $error');
        }
      }
    }
    if (!mounted) return;
    final sorted = games.values.toList()
      ..sort(
        (a, b) =>
            gameName(a).toLowerCase().compareTo(gameName(b).toLowerCase()),
      );
    setState(() {
      _games = sorted;
      _filterGames();
      _loading = false;
      if (errors.isNotEmpty) _error = 'Could not scan: ${errors.join('; ')}';
    });
    await _loadRecent();
  }

  Future<void> _addPath(String path) async {
    if (!_settingsReady || _loading || path.trim().isEmpty) return;
    try {
      final absolute = Directory(path.trim()).absolute.path;
      if (!await Directory(absolute).exists()) {
        throw FileSystemException('Folder does not exist', absolute);
      }
      final updated = [
        for (final f in _folders)
          RomFolder(f.path, enabled: f.enabled, shareSettings: f.shareSettings),
      ];
      final existing = updated.where(
        (f) => folderKey(f.path) == folderKey(absolute),
      );
      if (existing.isEmpty) {
        updated.add(RomFolder(absolute));
      } else {
        existing.first.enabled = true;
      }
      await _settings.save(updated);
      if (!mounted) return;
      setState(() {
        _folders = updated;
        _error = null;
      });
      _folder.clear();
      await _scan();
    } catch (error) {
      if (mounted) setState(() => _error = 'Could not add ROM folder: $error');
    }
  }

  Future<void> _choose() async {
    if (_choosing) return;
    setState(() => _choosing = true);
    try {
      await _restored;
      if (!mounted) return;
      final folder = Platform.isIOS
          ? await RomFolderAccess.pickIOSFolder()
          : await getDirectoryPath(confirmButtonText: 'Add ROM folder');
      if (mounted && folder != null) {
        await _scanInProgress;
        if (!mounted) return;
        await RomFolderAccess.remember(folder);
        await _addPath(folder);
      }
    } catch (error) {
      if (mounted) {
        setState(() => _error = 'Could not open folder picker: $error');
      }
    } finally {
      if (mounted) setState(() => _choosing = false);
    }
  }

  Future<void> _manage() async {
    final edited = [
      for (final f in _folders)
        RomFolder(f.path, enabled: f.enabled, shareSettings: f.shareSettings),
    ];
    final apply = await showDialog<bool>(
      context: context,
      builder: (context) => StatefulBuilder(
        builder: (context, update) => AlertDialog(
          insetPadding: const EdgeInsets.symmetric(
            horizontal: 16,
            vertical: 24,
          ),
          contentPadding: const EdgeInsets.fromLTRB(16, 16, 16, 0),
          title: const Text('Manage ROM folders'),
          content: SizedBox(
            width: 600,
            height: 320,
            child: edited.isEmpty
                ? const Center(child: Text('No ROM folders added.'))
                : ListView.builder(
                    itemCount: edited.length,
                    itemBuilder: (_, index) {
                      final folder = edited[index];
                      return Padding(
                        padding: const EdgeInsets.only(bottom: 16),
                        child: Column(
                          crossAxisAlignment: CrossAxisAlignment.stretch,
                          children: [
                            Row(
                              children: [
                                Checkbox(
                                  value: folder.enabled,
                                  onChanged: (value) =>
                                      update(() => folder.enabled = value!),
                                ),
                                const Expanded(child: Text('Enabled')),
                                IconButton(
                                  tooltip: 'Remove folder',
                                  icon: const Icon(Icons.delete_outline),
                                  onPressed: () =>
                                      update(() => edited.removeAt(index)),
                                ),
                              ],
                            ),
                            Text(folder.path, softWrap: true),
                            CheckboxListTile(
                              contentPadding: EdgeInsets.zero,
                              dense: true,
                              controlAffinity: ListTileControlAffinity.leading,
                              title: const Text(
                                'Share playtime through this ROM folder',
                              ),
                              subtitle: const Text(
                                'Newest copy wins. Sync this folder between devices.',
                              ),
                              value: folder.shareSettings,
                              onChanged: (value) =>
                                  update(() => folder.shareSettings = value!),
                            ),
                          ],
                        ),
                      );
                    },
                  ),
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.pop(context, false),
              child: const Text('Cancel'),
            ),
            FilledButton(
              onPressed: () => Navigator.pop(context, true),
              child: const Text('Save'),
            ),
          ],
        ),
      ),
    );
    if (apply != true || !mounted) return;
    try {
      await _settings.save(edited);
      if (!mounted) return;
      setState(() {
        _folders = edited;
        _error = null;
      });
      await _scan();
    } catch (error) {
      if (mounted) {
        setState(() => _error = 'Could not save ROM folders: $error');
      }
    }
  }

  Future<void> _play(String rom) async {
    if (_opening) return;
    final library = defaultNativeLibraryPath();
    setState(() => _opening = true);
    await Navigator.of(context).push(
      MaterialPageRoute<void>(
        builder: (_) => EmulationPage(
          library: library,
          rom: rom,
          settings: _settings,
          boxArtUrl: _covers?.match(rom)?.url,
        ),
      ),
    );
    if (mounted) {
      setState(() {
        _opening = false;
      });
      await _loadRecent();
    }
  }

  Future<void> _showRecentMenu(String rom, RenderBox boxArt) async {
    const menuWidth = 200.0;
    const menuHeight = kMinInteractiveDimension + 16;
    final overlay = Overlay.of(context).context.findRenderObject() as RenderBox;
    final center = overlay.globalToLocal(
      boxArt.localToGlobal(boxArt.size.center(Offset.zero)),
    );
    final reset = await showMenu<bool>(
      context: context,
      constraints: const BoxConstraints.tightFor(
        width: menuWidth,
        height: menuHeight,
      ),
      menuPadding: const EdgeInsets.symmetric(vertical: 8),
      position: RelativeRect.fromRect(
        // Matching the anchor and menu widths prevents showMenu's automatic
        // left/right growth direction from changing the horizontal position.
        Rect.fromLTWH(
          center.dx - menuWidth / 2,
          center.dy - menuHeight / 2,
          menuWidth,
          0,
        ),
        Offset.zero & overlay.size,
      ),
      items: const [
        PopupMenuItem(value: true, child: Text('Reset time played')),
      ],
    );
    if (reset != true || !mounted) return;
    try {
      await _settings.resetPlaytimeAsync(rom);
      await _loadRecent();
    } catch (error) {
      setState(() => _error = 'Could not reset time played: $error');
    }
  }

  Widget _gameTile(String rom, {bool showPlaytime = false}) {
    final cover = _covers?.match(rom);
    final boxArtKey = showPlaytime ? GlobalKey() : null;
    return Card(
      color: Colors.transparent,
      shadowColor: Colors.transparent,
      clipBehavior: Clip.antiAlias,
      child: InkWell(
        onTap: _opening ? null : () => _play(rom),
        onSecondaryTapUp: showPlaytime && !_opening
            ? (_) {
                final box = boxArtKey?.currentContext?.findRenderObject();
                if (box is RenderBox) {
                  _showRecentMenu(rom, box);
                }
              }
            : null,
        child: Padding(
          padding: const EdgeInsets.all(3),
          child: Column(
            children: [
              if (showPlaytime)
                Text(
                  formatPlaytime(_playtimes[folderKey(rom)] ?? 0),
                  textAlign: TextAlign.center,
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                  style: Theme.of(context).textTheme.labelLarge
                      ?.copyWith(color: Theme.of(context).colorScheme.primary),
                ),
              if (showPlaytime) const SizedBox(height: 4),
              Expanded(
                child: SizedBox(
                  key: boxArtKey,
                  width: double.infinity,
                  child: BoxArtImage(
                    url: cover?.url,
                    semanticLabel:
                        '${cover?.shortName ?? shortRomName(rom)} box art',
                  ),
                ),
              ),
              const SizedBox(height: 2),
              Tooltip(
                message: gameName(rom),
                child: Text(
                  shortRomName(rom),
                  textAlign: TextAlign.center,
                  maxLines: 2,
                  overflow: TextOverflow.ellipsis,
                  style: Theme.of(context).textTheme.titleSmall,
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _indexedGames(Widget Function(ScrollController, int) buildGames) {
    final letters = <String, int>{};
    for (var i = 0; i < _filteredGames.length; i++) {
      final name = gameName(_filteredGames[i]).trim().toUpperCase();
      final letter = name.isNotEmpty && RegExp(r'^[A-Z]').hasMatch(name)
          ? name[0]
          : '#';
      letters.putIfAbsent(letter, () => i);
    }
    final controller = _tileScroll;
    return KeyboardScrollArea(
      controller: controller,
      child: LayoutBuilder(
        builder: (context, bounds) {
          final columns = ((bounds.maxWidth - 40) / 248).ceil().clamp(1, 1000);
          return Row(
            children: [
              Expanded(
                child: LayoutBuilder(
                  builder: (context, constraints) {
                    return ScrollConfiguration(
                      behavior: ScrollConfiguration.of(context)
                          .copyWith(scrollbars: false),
                      child: Scrollbar(
                        controller: controller,
                        thumbVisibility: true,
                        child: buildGames(controller, columns),
                      ),
                    );
                  },
                ),
              ),
              const SizedBox(width: 16),
              SizedBox(
                width: 12,
                child: LayoutBuilder(
                  builder: (context, constraints) {
                    return SingleChildScrollView(
                      child: Column(
                        children: [
                          for (final letter
                              in '#ABCDEFGHIJKLMNOPQRSTUVWXYZ'.split(''))
                            SizedBox(
                              height: (constraints.maxHeight / 27).clamp(
                                18.0,
                                28.0,
                              ),
                              child: InkWell(
                                key: ValueKey('jump-$letter'),
                                onTap: !letters.containsKey(letter)
                                    ? null
                                    : () {
                                        if (!controller.hasClients) return;
                                        final index = letters[letter]!;

                                        final grid = _gridKey.currentContext
                                            ?.findRenderObject();
                                        final headerExtent =
                                            grid is RenderSliver
                                            ? grid
                                                  .constraints
                                                  .precedingScrollExtent
                                            : 0.0;
                                        final offset =
                                            headerExtent +
                                            (index ~/ columns) * 308.0;
                                        controller.animateTo(
                                          offset.clamp(
                                            0.0,
                                            controller.position.maxScrollExtent,
                                          ),
                                          duration: const Duration(
                                            milliseconds: 250,
                                          ),
                                          curve: Curves.easeOut,
                                        );
                                      },
                                child: Center(
                                  child: Text(
                                    letter,
                                    style: TextStyle(
                                      fontSize: 12,
                                      color: letters.containsKey(letter)
                                          ? Theme.of(context)
                                                .colorScheme
                                                .primary
                                          : Theme.of(context).disabledColor,
                                    ),
                                  ),
                                ),
                              ),
                            ),
                        ],
                      ),
                    );
                  },
                ),
              ),
            ],
          );
        },
      ),
    );
  }

  @override
  void dispose() {
    _tileScroll.dispose();
    _search.dispose();
    _folder.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) => Scaffold(
    appBar: AppBar(
      backgroundColor: Colors.indigoAccent,
      shadowColor: Colors.transparent,
      foregroundColor: Colors.white,
      title: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          const Text(
            'GBCEmulator',
            style: TextStyle(fontWeight: FontWeight.bold),
          ),
          FutureBuilder<PackageInfo>(
            future: _packageInfo,
            builder: (context, snapshot) {
              final version = snapshot.data?.version;
              if (version == null || version.isEmpty) {
                return const SizedBox.shrink();
              }
              return Padding(
                padding: const EdgeInsets.only(left: 7, top: 6),
                child: Text(
                  'v$version',
                  style: const TextStyle(
                    fontSize: 14,
                    fontWeight: FontWeight.bold,
                  ),
                ),
              );
            },
          ),
        ],
      ),
    ),
    body: Padding(
      padding: const EdgeInsets.all(20),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          Wrap(
            spacing: 12,
            runSpacing: 8,
            children: [
              FilledButton.icon(
                onPressed: _choosing ? null : _choose,
                icon: const Icon(Icons.folder_open),
                label: const Text('Add ROM folder'),
              ),
              FilledButton.icon(
                onPressed: _loading || !_settingsReady ? null : _manage,
                icon: const Icon(Icons.folder_copy_outlined),
                label: const Text('Manage ROM folders'),
              ),
              FilledButton.icon(
                onPressed: _loading ? null : _scan,
                icon: const Icon(Icons.refresh),
                label: const Text('Scan folder'),
              ),
            ],
          ),
          const SizedBox(height: 12),
          Wrap(
            spacing: 12,
            runSpacing: 8,
            crossAxisAlignment: WrapCrossAlignment.center,
            children: [
              SegmentedButton<GameRegion>(
                showSelectedIcon: false,
                style: SegmentedButton.styleFrom(
                  selectedBackgroundColor: Theme.of(context)
                      .colorScheme
                      .primary,
                  selectedForegroundColor: Theme.of(context)
                      .colorScheme
                      .onPrimary,
                  foregroundColor: Theme.of(context).colorScheme.primary,
                  side: BorderSide(
                    color: Theme.of(context).colorScheme.primary,
                  ),
                ),
                segments: const [
                  ButtonSegment(value: GameRegion.all, label: Text('All')),
                  ButtonSegment(value: GameRegion.usa, label: Text('USA')),
                  ButtonSegment(value: GameRegion.eu, label: Text('EU')),
                  ButtonSegment(value: GameRegion.jpn, label: Text('JPN')),
                ],
                selected: {_region},
                onSelectionChanged: (selection) {
                  _region = selection.first;
                  _searchChanged(_search.text);
                },
              ),
              SizedBox(
                width: 260,
                child: TextField(
                  controller: _search,
                  onChanged: _searchChanged,
                  decoration: InputDecoration(
                    hintText: 'Search games',
                    isDense: true,
                    border: const OutlineInputBorder(),
                    prefixIcon: const Icon(Icons.search),
                    suffixIcon: _search.text.isEmpty
                        ? null
                        : IconButton(
                            tooltip: 'Clear search',
                            icon: const Icon(Icons.close),
                            onPressed: () {
                              _search.clear();
                              _searchChanged('');
                            },
                          ),
                  ),
                ),
              ),
            ],
          ),
          const SizedBox(height: 12),
          Expanded(
            child: _indexedGames(
              (controller, columns) => CustomScrollView(
                key: const PageStorageKey('game-library-scroll'),
                controller: controller,
                slivers: [
                  SliverToBoxAdapter(
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.stretch,
                      children: [
                        if (_search.text.trim().isEmpty &&
                            _recentGames.isNotEmpty) ...[
                          Text(
                            'Recently Played',
                            style: TextStyle(
                              fontWeight: FontWeight.bold,
                              fontSize: 24,
                            ),
                          ),
                          const SizedBox(height: 6),
                          SizedBox(
                            height: 300,
                            child: ListView.separated(
                              key: const PageStorageKey('recently-played'),
                              scrollDirection: Axis.horizontal,
                              itemCount: _recentGames.length,
                              separatorBuilder: (_, index) =>
                                  const SizedBox(width: 8),
                              itemBuilder: (context, index) {
                                final rom = _recentGames[index];
                                return SizedBox(
                                  width: 240,
                                  child: _gameTile(rom, showPlaytime: true),
                                );
                              },
                            ),
                          ),
                          const SizedBox(height: 12),
                        ],
                        Text(
                          _error ?? '${_filteredGames.length} Games Found',
                          style: TextStyle(
                            fontWeight: FontWeight.bold,
                            fontSize: 24,
                          ),
                        ),
                        if (_coverError != null) Text(_coverError!),
                      ],
                    ),
                  ),
                  if (_loading)
                    const SliverFillRemaining(
                      hasScrollBody: false,
                      child: Center(child: CircularProgressIndicator()),
                    )
                  else
                    SliverGrid(
                      key: _gridKey,
                      gridDelegate: SliverGridDelegateWithFixedCrossAxisCount(
                        crossAxisCount: columns,
                        mainAxisExtent: 300,
                        crossAxisSpacing: 8,
                        mainAxisSpacing: 8,
                      ),
                      delegate: SliverChildBuilderDelegate(
                        (context, index) => _gameTile(_filteredGames[index]),
                        childCount: _filteredGames.length,
                      ),
                    ),
                ],
              ),
            ),
          ),
        ],
      ),
    ),
  );
}
