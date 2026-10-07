import Cocoa
import FlutterMacOS

class MainFlutterWindow: NSWindow {
  private var folderChannel: FlutterMethodChannel?
  private var accessedFolders: [String: URL] = [:]
  private let bookmarkKey = "GBCEmulator.romFolderBookmarks"

  private func rememberFolder(_ path: String) throws {
    let url = URL(fileURLWithPath: path, isDirectory: true)
    let bookmark = try url.bookmarkData(options: .withSecurityScope,
      includingResourceValuesForKeys: nil, relativeTo: nil)
    var bookmarks = UserDefaults.standard.dictionary(forKey: bookmarkKey) ?? [:]
    bookmarks[path] = bookmark
    UserDefaults.standard.set(bookmarks, forKey: bookmarkKey)
    if accessedFolders[path] == nil && url.startAccessingSecurityScopedResource() {
      accessedFolders[path] = url
    }
  }

  private func restoreFolders() -> [String] {
    var warnings: [String] = []
    var bookmarks = UserDefaults.standard.dictionary(forKey: bookmarkKey) ?? [:]
    for (path, value) in bookmarks {
      if accessedFolders[path] != nil { continue }
      do {
        guard let data = value as? Data else {
          throw NSError(domain: "ROMFolderAccess", code: 1)
        }
        var stale = false
        let url = try URL(resolvingBookmarkData: data,
          options: [.withSecurityScope, .withoutUI], relativeTo: nil,
          bookmarkDataIsStale: &stale)
        guard url.startAccessingSecurityScopedResource() else {
          throw NSError(domain: "ROMFolderAccess", code: 2)
        }
        // Keep access alive for scans, native emulation, saves, and settings sync.
        accessedFolders[path] = url
        if stale {
          bookmarks[path] = try url.bookmarkData(options: .withSecurityScope,
            includingResourceValuesForKeys: nil, relativeTo: nil)
        }
      } catch {
        warnings.append("Could not restore access to \(path). Reconnect the drive and select it again with Add ROM Folder. \(error.localizedDescription)")
      }
    }
    UserDefaults.standard.set(bookmarks, forKey: bookmarkKey)
    return warnings
  }

  deinit {
    for url in accessedFolders.values { url.stopAccessingSecurityScopedResource() }
  }

  override func awakeFromNib() {
    let flutterViewController = FlutterViewController()
    let windowFrame = self.frame
    self.contentViewController = flutterViewController
    self.setFrame(windowFrame, display: true)

    RegisterGeneratedPlugins(registry: flutterViewController)

    folderChannel = FlutterMethodChannel(name: "gbcemulator/rom_folder_access",
      binaryMessenger: flutterViewController.engine.binaryMessenger)
    folderChannel?.setMethodCallHandler { [weak self] call, result in
      guard let self = self else { result(FlutterMethodNotImplemented); return }
      switch call.method {
      case "restore":
        result(self.restoreFolders())
      case "remember":
        guard let path = call.arguments as? String else {
          result(FlutterError(code: "invalid_path", message: "Missing folder path", details: nil))
          return
        }
        do {
          try self.rememberFolder(path)
          result(nil)
        } catch {
          result(FlutterError(code: "bookmark_failed", message: error.localizedDescription, details: path))
        }
      default:
        result(FlutterMethodNotImplemented)
      }
    }

    super.awakeFromNib()
  }
}
