<p align="center">
  <img src="assets/luma-logo/luma-logo.svg" width="128" alt="Luma">
</p>

<div align="center">
  <h1>Luma</h1>
  <p>Multi-application firmware for the M5Stack Cardputer ADV</p>
</div>

<p align="center">
  <a href="https://github.com/zhoux77899/luma/releases"><img src="https://img.shields.io/github/v/release/zhoux77899/luma?display_name=tag&include_prereleases=true&label=release" alt="Latest release"></a>
  <a href="https://github.com/zhoux77899/luma/blob/main/LICENSE"><img src="https://img.shields.io/github/license/zhoux77899/luma" alt="License"></a>
  <img src="https://img.shields.io/badge/hardware-M5Stack%20Cardputer%20ADV-2ea44f" alt="Hardware: M5Stack Cardputer ADV">
  <img src="https://img.shields.io/badge/chip-ESP32--S3-2ea44f" alt="Chip: ESP32-S3">
</p>

Luma is statically compiled firmware for the M5Stack Cardputer ADV. It boots through a
short logo screen into Launcher, where the built-in Settings, About, and Notes Apps are
available. Platform-independent Core contracts are reused by the Cardputer firmware and
the host-side SDL preview.

## Choose a path

| Goal | Start here |
| --- | --- |
| Use Luma on a Cardputer ADV | [Flash a release](#flash-a-release) |
| Build or test the firmware | [Build and test](#build-and-test) |
| Upload a local build to hardware | [Upload and monitor](#upload-and-monitor) |
| Inspect the UI without hardware | [Run the SDL preview](#run-the-sdl-preview) |

## System and dependency requirements

Use this table as the single source for system, dependency, and hardware requirements.
The workflow sections below assume these requirements are already satisfied.

| Workflow | System requirements | Project dependencies | Hardware |
| --- | --- | --- | --- |
| Build firmware | Python 3.11 | PlatformIO 6.1.19 from [`requirements.txt`](requirements.txt); Arduino and `M5Cardputer` are resolved by PlatformIO | None |
| Run native tests | Python 3.11 and a C++17 host compiler | PlatformIO 6.1.19 | None |
| Flash a release | Python 3.11 | `esptool` | M5Stack Cardputer ADV and a data-capable USB-C cable |
| Upload and monitor | Python 3.11 | PlatformIO 6.1.19 | M5Stack Cardputer ADV and a data-capable USB-C cable |
| Run the SDL preview | CMake 3.16 or newer and a C++17 compiler | Bootstrapped [vcpkg](https://github.com/microsoft/vcpkg); SDL2 from `tools/sdl-preview/vcpkg.json` | None |

Install the development dependency from the repository root. The same command works on
Windows, macOS, and Linux when `python` points to Python 3.11:

```bash
python -m pip install -r requirements.txt
```

Manual release flashing additionally requires `esptool`:

```bash
python -m pip install esptool
```

If `python` does not point to Python 3.11, use `py -3.11` on Windows or `python3.11` on
macOS/Linux in these commands. PlatformIO downloads the Arduino framework and the
`M5Cardputer` dependency on the first firmware build.

On Windows, if the native test compiler is not found, add PlatformIO's bundled MinGW
toolchain to the current PowerShell session before running `pio test -e native`:

```powershell
$mingwBin = Join-Path $env:USERPROFILE ".platformio\packages\toolchain-gccmingw32\bin"
$env:PATH = $mingwBin + [System.IO.Path]::PathSeparator + $env:PATH
```

The SDL preview uses one vcpkg triplet per host:

| Host | Triplet |
| --- | --- |
| Windows x64 | `x64-windows` |
| Apple Silicon | `arm64-osx` |
| Intel macOS | `x64-osx` |
| Linux x64 | `x64-linux` |

Install and bootstrap vcpkg using its [official instructions](https://learn.microsoft.com/vcpkg/get_started/overview)
before running the SDL setup commands.

## Included Apps

- **Launcher** opens the registered Apps with directional navigation and confirm.
- **Settings** persists brightness, Volume, and Dark/Light Theme preference.
- **Notes** edits a bounded plain-text Notes document stored on the device and autosaves
  after edits.
- **About** shows the installed Luma version, hardware target, build environment, and
  license.

## Flash a release

Release images are published on the [GitHub Releases page](https://github.com/zhoux77899/luma/releases).
They target the M5Stack Cardputer ADV with an ESP32-S3 and 8 MB flash. Do not flash them
to another board.

For the shortest path, download the release's merged image and `SHA256SUMS`.

### Verify the download

Use the command for your platform and compare the result with the matching entry in
`SHA256SUMS`:

#### Windows PowerShell

```powershell
Get-FileHash .\luma-cardputer-vX.Y.Z-merged.bin -Algorithm SHA256
```

#### macOS

```bash
shasum -a 256 luma-cardputer-vX.Y.Z-merged.bin
```

#### Linux

```bash
sha256sum --ignore-missing -c SHA256SUMS
```

Install `esptool` as described in [System and dependency requirements](#system-and-dependency-requirements).

### Enter download mode

Power off the Cardputer ADV, hold **G0** while reconnecting a data-capable USB-C cable,
then release **G0**.

### Flash the merged image

The merged image is the complete 8 MB flash image and must be written at address
`0x00000000`:

```bash
esptool.py --chip esp32s3 write_flash 0x00000000 luma-cardputer-vX.Y.Z-merged.bin
```

If `esptool.py` is not on `PATH`, replace it with `py -3.11 -m esptool` on Windows or
`python3.11 -m esptool` on macOS/Linux. If automatic port detection fails, add
`--port <PORT>` (for example, `COM5`, `/dev/cu.usbmodem*`, or `/dev/ttyUSB0`).

Advanced users can download the release Flash package. It contains the split images,
their addresses in `flash.json`, `FLASHING.md`, and checksums.

## Build and test

Build the Cardputer firmware and run the platform-independent Core tests:

```bash
pio run -e m5stack-cardputer
pio test -e native
```

The Cardputer environment uses the generic `esp32-s3-devkitc-1` board definition for
the Cardputer ADV, LittleFS, USB CDC on boot, and C++17. The native environment excludes
the Cardputer platform adapters and tests the shared Core on the host. See [System and
dependency requirements](#system-and-dependency-requirements) for the Windows compiler
workaround.

## Upload and monitor

Connect the Cardputer ADV with a data-capable USB-C cable and upload the local build:

```bash
pio run -e m5stack-cardputer --target upload
```

If the board is not detected, power it off, hold **G0** while reconnecting USB-C,
release **G0**, and retry. Uploading changes the connected device; use a release image
if you do not intend to build from source.

The serial monitor uses 115200 baud:

```bash
pio device monitor
```

A successful cold boot should show the Boot screen and then Launcher. The serial output
starts with `[BOOT] Luma Cardputer ADV started`; App transitions emit `[APP]` lines and
key diagnostics use the `[KEY]` prefix.

## Run the SDL preview

The SDL preview displays the same Core and Apps on the host. It is a secondary validation
path, not a separate product. See [System and dependency requirements](#system-and-dependency-requirements)
for the CMake, compiler, vcpkg, and host-triplet requirements.

### Install SDL2

Set `VCPKG_ROOT` and install SDL2 with the command for your platform.

#### Windows PowerShell

```powershell
$env:VCPKG_ROOT = "C:\path\to\vcpkg"
& "$env:VCPKG_ROOT\vcpkg.exe" install --triplet x64-windows --manifest-dir tools/sdl-preview
```

#### macOS

Use `arm64-osx` on Apple Silicon. Replace it with `x64-osx` on Intel Macs.

```bash
export VCPKG_ROOT="/path/to/vcpkg"
"$VCPKG_ROOT/vcpkg" install --triplet arm64-osx --manifest-dir tools/sdl-preview
```

#### Linux

```bash
export VCPKG_ROOT="/path/to/vcpkg"
"$VCPKG_ROOT/vcpkg" install --triplet x64-linux --manifest-dir tools/sdl-preview
```

### Configure the project

Configure the project with the command for your platform.

#### Windows PowerShell

```powershell
cmake -S tools/sdl-preview -B build/sdl-preview -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
```

#### macOS / Linux

```bash
cmake -S tools/sdl-preview -B build/sdl-preview \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
```

### Build the preview

```bash
cmake --build build/sdl-preview --config Debug --target luma-sdl-preview
```

### Run the preview

Run the executable from the build directory so preview data is written to `data/`.

#### Windows PowerShell

```powershell
Set-Location build/sdl-preview
.\Debug\luma-sdl-preview.exe
```

#### macOS / Linux

```bash
cd build/sdl-preview
./luma-sdl-preview
```

The preview window is 960 x 540 with a fixed 240 x 135 logical canvas and integer 4x
nearest-neighbor scaling. Arrow keys, Enter, Escape, Backspace/Delete, Page Up/Page Down,
and printable characters map to `InputFrame` values.

## Project layout

- `include/luma` and `src/luma`: platform-independent Core, Apps, and UI.
- `src/luma/platform/cardputer`: Cardputer hardware adapters and factory.
- `src/luma/platform/host`: host storage, clock, audio, and diagnostics adapters.
- `tools/sdl-preview`: CMake target and SDL display/input adapters.
- `test`: native Core tests.
- `partitions/luma-8mb.csv`: the Cardputer ADV 8 MB partition layout.

Architecture decisions and visual design tokens are documented in [`docs/adr`](docs/adr)
and [`DESIGN.md`](DESIGN.md).

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md) for commit and pull request conventions.
Coding agents should read [`AGENTS.md`](AGENTS.md) before editing this repository.

## License

Luma is released under the [MIT License](LICENSE).
