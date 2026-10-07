[CmdletBinding()]
param(
  [string]$NativeBuildDirectory = '',
  [string]$Flutter = 'flutter'
)

$ErrorActionPreference = 'Stop'
$guiRoot = Split-Path -Parent $PSScriptRoot
$versionLines = @(Get-Content -LiteralPath (Join-Path $guiRoot 'pubspec.yaml') |
  Where-Object { $_ -match '^version\s*:' })
if ($versionLines.Count -ne 1) {
  throw 'Expected exactly one version field in the GUI pubspec.yaml.'
}
$version = ($versionLines[0] -replace '^version\s*:\s*', '' -replace '\s+#.*$', '').Trim().Trim("'`"")
if ($version -notmatch '^\d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?(?:\+[0-9A-Za-z.-]+)?$') {
  throw "Invalid package version in pubspec.yaml: $version"
}
$version = ($version -split '\+', 2)[0]
$packageDirectory = Join-Path $guiRoot "build/packages/GBCEmulator-Windows-x64-$version"
$zip = "$packageDirectory.zip"
if ((Test-Path -LiteralPath $packageDirectory) -or (Test-Path -LiteralPath $zip)) {
  throw "Package $version already exists. Move the existing outputs or update pubspec.yaml's version before packaging again."
}
$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $guiRoot '../..'))
if (-not $NativeBuildDirectory) {
  $NativeBuildDirectory = Join-Path $repositoryRoot 'build/flutter-ffi'
}
$NativeBuildDirectory = (Resolve-Path -LiteralPath $NativeBuildDirectory).Path
if (-not (Test-Path -LiteralPath (Join-Path $NativeBuildDirectory 'CMakeCache.txt'))) {
  throw 'Configure the native CMake build with BUILD_FLUTTER_FFI=ON first (see README).'
}

$previousNativeBuildDirectory = $env:GBC_NATIVE_BUILD_DIR
$env:GBC_NATIVE_BUILD_DIR = $NativeBuildDirectory
Push-Location $guiRoot
try {
  # Resolve dependencies with flutter pub get before running this script.
  # --no-pub also preserves this checkout's existing Windows plugin junctions.
  & $Flutter build windows --release --no-pub
  if ($LASTEXITCODE -ne 0) { throw 'Flutter Release build failed.' }
  $releaseDirectory = Join-Path $guiRoot 'build/windows/x64/runner/Release'
  foreach ($required in @('GBCEmulator.exe', 'flutter_windows.dll', 'data', 'gbcemulator_ffi.dll', 'zip.dll')) {
    if (-not (Test-Path -LiteralPath (Join-Path $releaseDirectory $required))) {
      throw "Missing Flutter Release output: $required"
    }
  }

  # Fresh staging avoids stale emulator DLLs and excludes ROM/save/log files.
  New-Item -ItemType Directory -Path $packageDirectory -Force | Out-Null
  Copy-Item -LiteralPath (Join-Path $releaseDirectory 'GBCEmulator.exe') -Destination $packageDirectory
  Get-ChildItem -LiteralPath $releaseDirectory -Filter '*.dll' -File |
    Copy-Item -Destination $packageDirectory
  Copy-Item -LiteralPath (Join-Path $releaseDirectory 'data') -Destination $packageDirectory -Recurse
  $nativeManifest = Join-Path $releaseDirectory 'native_assets.json'
  if (Test-Path -LiteralPath $nativeManifest) {
    Copy-Item -LiteralPath $nativeManifest -Destination $packageDirectory
  }

  Set-Content -LiteralPath (Join-Path $packageDirectory 'README.txt') -Encoding utf8 -Value @(
    'Run GBCEmulator.exe from this folder. Keep the data folder and all DLLs together.'
    'Requires a compatible Microsoft Visual C++ x64 Redistributable.'
    'Add your own ROM folders. ROMs and personal settings are not included.'
  )
  Compress-Archive -LiteralPath $packageDirectory -DestinationPath $zip
  Write-Host "Portable app: $packageDirectory"
  Write-Host "Distribution ZIP: $zip"
}
finally {
  Pop-Location
  $env:GBC_NATIVE_BUILD_DIR = $previousNativeBuildDirectory
}
