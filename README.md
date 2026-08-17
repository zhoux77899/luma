<p align="center">
  <img src="assets/luma-logo/luma-logo.svg" width="128" alt="Luma">
</p>

<div align="center">
  <h1>Luma</h1>
  <p>for M5Stack Cardputer ADV</p>
</div>

<p align="center">
  <a href="https://github.com/zhoux77899/luma/releases"><img src="https://img.shields.io/github/v/release/zhoux77899/luma?display_name=tag&include_prereleases=true&label=release" alt="Latest release"></a>
  <a href="https://github.com/zhoux77899/luma/blob/main/LICENSE"><img src="https://img.shields.io/github/license/zhoux77899/luma" alt="License"></a>
  <img src="https://img.shields.io/badge/hardware-M5Stack%20Cardputer%20ADV-2ea44f" alt="Hardware: M5Stack Cardputer ADV">
  <img src="https://img.shields.io/badge/chip-ESP32--S3-2ea44f" alt="Chip: ESP32-S3">
</p>

Luma is a statically compiled multi-application firmware for the M5Stack Cardputer ADV.
It boots into Launcher, keeps hardware APIs behind platform adapters, and shares the
same Core with the host SDL preview.

## Features

- Boot screen followed by Launcher navigation for the built-in Settings, About, and
  Notes Apps.
- Settings for brightness, sound, and Dark/Light appearance preferences.
- Bounded plain-text Notes document with persistent storage and autosave behavior.
- Platform-independent Core contracts for App lifecycle, normalized InputFrame values,
  display, storage, clock, diagnostics, and audio services.
- Cardputer hardware adapters for the ESP32-S3 target and host adapters for the SDL
  preview.

## Flash a release

Release images are available from the [GitHub Releases page](https://github.com/zhoux77899/luma/releases).
The merged image is built for the M5Stack Cardputer ADV with an ESP32-S3 and 8 MB
flash. Do not flash it to another board.

### 1. Download and verify the image

Download these two assets for the release you want:

- `luma-cardputer-vX.Y.Z-merged.bin`
- `SHA256SUMS`

Verify the merged image before flashing:

```powershell
# Windows PowerShell
Get-FileHash .\luma-cardputer-vX.Y.Z-merged.bin -Algorithm SHA256
```

```bash
# macOS
shasum -a 256 luma-cardputer-vX.Y.Z-merged.bin

# Linux
sha256sum --ignore-missing -c SHA256SUMS
```

Compare the Windows and macOS output with the matching line in `SHA256SUMS`.

### 2. Install esptool

Install `esptool` in the Python environment used for flashing:

```powershell
# Windows PowerShell
py -3.11 -m pip install esptool
```

```bash
# macOS / Linux
python3.11 -m pip install esptool
```

### 3. Enter download mode

Power off the Cardputer ADV, hold **G0** while reconnecting a data-capable USB-C
cable, then release **G0**.

### 4. Flash the merged `.bin`

Write the complete image at address `0x00000000`:

```powershell
# Windows PowerShell
esptool.py --chip esp32s3 write_flash 0x00000000 .\luma-cardputer-vX.Y.Z-merged.bin
```

```bash
# macOS / Linux
esptool.py --chip esp32s3 write_flash 0x00000000 ./luma-cardputer-vX.Y.Z-merged.bin
```

If `esptool.py` is not on `PATH`, use `py -3.11 -m esptool` on Windows or
`python3.11 -m esptool` on macOS/Linux in the same command. If automatic port
detection fails, add `--port <PORT>`, such as `COM5`, `/dev/cu.usbmodem*`, or
`/dev/ttyUSB0`.

The `luma-cardputer-vX.Y.Z-flash.zip` Flash package contains the split images,
`flash.json`, `FLASHING.md`, and an internal checksum file for advanced flashing
workflows.

Each release also publishes Release-configuration SDL preview zips. `SHA256SUMS`
covers the Merged image, the Flash package, and these preview archives:

- `luma-sdl-preview-vX.Y.Z-windows-x64.zip`
- `luma-sdl-preview-vX.Y.Z-macos-arm64.zip`
- `luma-sdl-preview-vX.Y.Z-macos-x64.zip`
- `luma-sdl-preview-vX.Y.Z-linux-x64.zip`

## CI artifacts

Pull requests and pushes to `main` upload GitHub Actions artifacts (retained 14
days):

- `cardputer-flash` — Flash package zip, Merged image, and `SHA256SUMS`
- `sdl-preview-windows-x64`
- `sdl-preview-macos-arm64`
- `sdl-preview-macos-x64`
- `sdl-preview-linux-x64`

CI Cardputer and SDL preview builds stamp About as `{version}+g{shortsha}`, for
example `0.1+gabcdef1`. Artifact file names use a hyphen in place of `+`.

## Development

The firmware uses PlatformIO, Python 3.11, the Arduino framework, and C++17.
Install the PlatformIO version pinned in `requirements.txt` from a Python 3.11
installation:

```powershell
# Windows PowerShell
py -3.11 -m pip install -r requirements.txt
```

```bash
# macOS / Linux
python3.11 -m pip install -r requirements.txt
```

Run the following commands from the repository root:

```bash
pio run -e m5stack-cardputer
pio test -e native
```

The `native` environment requires a host C++17 compiler. On Windows, add the
PlatformIO MinGW package to the current PowerShell session when needed:

```powershell
$mingwBin = Join-Path $env:USERPROFILE ".platformio\packages\toolchain-gccmingw32\bin"
$env:PATH = $mingwBin + [System.IO.Path]::PathSeparator + $env:PATH
pio test -e native
```

On macOS and Linux, make a C++17 compiler available on `PATH`. The checked-in native
environment selects the MinGW toolchain package, so the SDL preview below is the
portable host-side validation path when that package is not available on your host.

### Upload a local build

Connect the Cardputer ADV with a data-capable USB-C cable and run:

```powershell
# Windows PowerShell
pio run -e m5stack-cardputer --target upload
```

```bash
# macOS / Linux
pio run -e m5stack-cardputer --target upload
```

If the board cannot be detected, power it off, hold **G0** while reconnecting USB-C,
release **G0**, and retry the upload.

### Serial monitor

The firmware uses 115200 baud:

```powershell
# Windows PowerShell
pio device monitor
```

```bash
# macOS / Linux
pio device monitor
```

Cold boot prints `[BOOT] Luma Cardputer ADV started`, shows the boot screen, and then
enters Launcher. App transitions emit `[APP]` lines. Typed keys are reported as
`[KEY]` diagnostics.

## SDL preview

The SDL preview runs the same Core and Apps on the host. CI and GitHub Releases
publish Release-configuration binaries for Windows x64, macOS arm64, macOS x64,
and Linux x64. Local builds below still use Debug.

The local workflow requires CMake 3.16 or newer, a C++17 compiler, vcpkg, and
`VCPKG_ROOT`. Install and bootstrap vcpkg using its official instructions, set
`VCPKG_ROOT` to that checkout, and run the commands for your platform from the
repository root.

### Windows (PowerShell)

```powershell
$env:VCPKG_ROOT = "C:\path\to\vcpkg"
& "$env:VCPKG_ROOT\vcpkg.exe" install --triplet x64-windows --manifest-dir tools/sdl-preview
cmake -S tools/sdl-preview -B build/sdl-preview `
    -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build/sdl-preview --config Debug --target luma-sdl-preview
Set-Location build/sdl-preview
.\Debug\luma-sdl-preview.exe
```

### macOS

Use `arm64-osx` on Apple Silicon or change the triplet to `x64-osx` on Intel Macs:

```bash
export VCPKG_ROOT="/path/to/vcpkg"
"$VCPKG_ROOT/vcpkg" install --triplet arm64-osx --manifest-dir tools/sdl-preview
cmake -S tools/sdl-preview -B build/sdl-preview \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build/sdl-preview --config Debug --target luma-sdl-preview
cd build/sdl-preview
./luma-sdl-preview
```

### Linux

```bash
export VCPKG_ROOT="/path/to/vcpkg"
"$VCPKG_ROOT/vcpkg" install --triplet x64-linux --manifest-dir tools/sdl-preview
cmake -S tools/sdl-preview -B build/sdl-preview \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build/sdl-preview --config Debug --target luma-sdl-preview
cd build/sdl-preview
./luma-sdl-preview
```

Run the executable from the build directory so preview data lands in `data/`. The
window is 960 x 540 with a fixed 240 x 135 logical canvas and integer 4x
nearest-neighbor scaling. Arrow keys, Enter, Escape, Backspace/Delete, and printable
characters map to `InputFrame`.

## Project layout

- `include/luma` and `src/luma`: platform-independent Core, Apps, and UI.
- `src/luma/platform/cardputer`: Cardputer hardware adapters and factory.
- `src/luma/platform/host`: host storage, clock, audio, and diagnostics adapters.
- `tools/sdl-preview`: CMake target and SDL display/input adapters.
- `test`: native Core tests.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for commit and pull request conventions.

## License

Luma is released under the [MIT License](LICENSE).
