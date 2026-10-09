import Flutter
import UIKit
import UniformTypeIdentifiers

@main
@objc class AppDelegate: FlutterAppDelegate, FlutterImplicitEngineDelegate, UIDocumentPickerDelegate {
  private var folderChannel: FlutterMethodChannel?
  private var pickerResult: FlutterResult?
  private var folders: [String: URL] = [:]
  private let bookmarkKey = "GBCEmulator.romFolderBookmarks"

  private func remember(_ url: URL) throws {
    if folders[url.path] == nil {
      guard url.startAccessingSecurityScopedResource() else {
        throw NSError(domain: "ROMFolderAccess", code: 1,
          userInfo: [NSLocalizedDescriptionKey: "Could not access the selected folder"])
      }
      folders[url.path] = url
    }
    // iOS bookmarks inherit the document picker's security scope.
    let data = try url.bookmarkData(options: [], includingResourceValuesForKeys: nil, relativeTo: nil)
    var bookmarks = UserDefaults.standard.dictionary(forKey: bookmarkKey) ?? [:]
    bookmarks[url.path] = data
    UserDefaults.standard.set(bookmarks, forKey: bookmarkKey)
  }

  private func restoreFolders() -> [String] {
    var warnings: [String] = []
    for (path, value) in UserDefaults.standard.dictionary(forKey: bookmarkKey) ?? [:] {
      if folders[path] != nil { continue }
      do {
        guard let data = value as? Data else { throw NSError(domain: "ROMFolderAccess", code: 2) }
        var stale = false
        let url = try URL(resolvingBookmarkData: data, options: [.withoutUI],
          relativeTo: nil, bookmarkDataIsStale: &stale)
        try remember(url)
      } catch {
        warnings.append("Select \(path) again with Add ROM Folder to restore access.")
      }
    }
    return warnings
  }

  func documentPicker(_ controller: UIDocumentPickerViewController, didPickDocumentsAt urls: [URL]) {
    let result = pickerResult
    pickerResult = nil
    guard let url = urls.first else { result?(nil); return }
    do {
      try remember(url)
      result?(url.path)
    } catch {
      result?(FlutterError(code: "folder_access_failed", message: error.localizedDescription, details: nil))
    }
  }

  func documentPickerWasCancelled(_ controller: UIDocumentPickerViewController) {
    pickerResult?(nil)
    pickerResult = nil
  }
  override func application(
    _ application: UIApplication,
    didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?
  ) -> Bool {
    return super.application(application, didFinishLaunchingWithOptions: launchOptions)
  }

  func didInitializeImplicitFlutterEngine(_ engineBridge: FlutterImplicitEngineBridge) {
    GeneratedPluginRegistrant.register(with: engineBridge.pluginRegistry)
    folderChannel = FlutterMethodChannel(name: "gbcemulator/rom_folder_access",
      binaryMessenger: engineBridge.applicationRegistrar.messenger())
    folderChannel?.setMethodCallHandler { [weak self] call, result in
      guard let self = self else { result(FlutterMethodNotImplemented); return }
      switch call.method {
      case "settingsPath":
        do {
          let directory = try FileManager.default.url(for: .applicationSupportDirectory,
            in: .userDomainMask, appropriateFor: nil, create: true)
            .appendingPathComponent("GBCEmulator", isDirectory: true)
          try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
          result(directory.appendingPathComponent("settings.json").path)
        } catch {
          result(FlutterError(code: "settings_path_failed", message: error.localizedDescription, details: nil))
        }
      case "restore": result(self.restoreFolders())
      case "pick":
        guard self.pickerResult == nil else {
          result(FlutterError(code: "picker_busy", message: "Folder picker is already open", details: nil))
          return
        }
        let scene = UIApplication.shared.connectedScenes.compactMap { $0 as? UIWindowScene }
          .first { $0.activationState == .foregroundActive }
        guard var presenter = scene?.windows.first(where: { $0.isKeyWindow })?.rootViewController else {
          result(FlutterError(code: "no_window", message: "No active window for the folder picker", details: nil))
          return
        }
        while let presented = presenter.presentedViewController { presenter = presented }
        let picker = UIDocumentPickerViewController(forOpeningContentTypes: [.folder], asCopy: false)
        picker.delegate = self
        self.pickerResult = result
        presenter.present(picker, animated: true)
      case "remember": result(nil) // pick already saved the original scoped URL.
      default: result(FlutterMethodNotImplemented)
      }
    }
  }
}
