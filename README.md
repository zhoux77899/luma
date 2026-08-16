<p align="center">
  <img src="assets/luma-logo/luma-logo.svg" width="128" alt="Luma">
</p>

<div align="center">
  <h1>Luma</h1>
  <p>for M5Stack Cardputer ADV</p>
</div>

Luma is a statically compiled multi-application firmware for the M5Stack Cardputer ADV.
The current milestone is the platform adapter seam: Cardputer firmware and the host
SDL preview share Core, boot into Launcher, and keep hardware APIs behind adapters.

## Build

PlatformIO Core is installed in the user environment on Windows. If `pio` is not on
PATH, run the bundled executable directly:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run
```

Host Core tests (requires a host `g++` on PATH). On Windows, PlatformIO's
MinGW package is enough:

```powershell
$env:PATH = "$env:USERPROFILE\.platformio\packages\toolchain-gccmingw32\bin;" + $env:PATH
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" test -e native
```

## SDL preview

The host preview reuses Core and Launcher. It needs CMake, a C++17 compiler, vcpkg,
and `VCPKG_ROOT`. Install SDL2, then configure and build from the repository root:

```powershell
vcpkg install --triplet x64-windows --manifest-dir tools/sdl-preview
cmake -S tools/sdl-preview -B build/sdl-preview `
    -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build/sdl-preview --config Debug --target luma-sdl-preview
```

Run the executable from the build directory so preview data lands in `data/`:

```powershell
Set-Location build/sdl-preview
.\Debug\luma-sdl-preview.exe
```

The window is 960 x 540 with a fixed 240 x 135 logical canvas and integer 4x
nearest-neighbor scaling. Arrow keys, Enter, Escape, Backspace/Delete, and
printable characters map to `InputFrame`. Host-only dependencies stay out of the
firmware build.

## CI and releases

GitHub Actions runs the native PlatformIO tests and the Cardputer firmware build
for every pull request. The same checks run for pushes to `main` and merge queue
entries. CI never uploads to hardware; the Cardputer ADV smoke check remains a
manual device validation.

Push a SemVer tag to create a GitHub Release automatically:

```bash
git tag -a v0.1.0 -m "Release v0.1.0"
git push origin v0.1.0
```

Tags use the form `vX.Y.Z` or `vX.Y.Z-rc.1`. Each release provides a split-image
flash package, an 8 MB merged image, and `SHA256SUMS`. The package contains
`FLASHING.md` and `flash.json` with the ESP32-S3 flash addresses.

## Upload

Connect the Cardputer ADV with a data-capable USB-C cable and run:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run --target upload
```

If the board cannot be detected, turn it off, hold **G0** while reconnecting USB-C,
release **G0**, and retry the upload.

## Serial monitor

The firmware uses 115200 baud:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" device monitor
```

Cold boot prints `[BOOT] Luma Cardputer ADV started`, shows the boot screen, then
enters Launcher. App transitions emit `[APP]` lines. Typed keys are reported as
`[KEY]` diagnostics; they are no longer echoed onto the boot screen.
