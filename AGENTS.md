# Luma Agent Guide

## Project purpose

Luma is an Arduino/C++ firmware project for the M5Stack Cardputer ADV. The current
milestone is the platform adapter seam: Luma boots into Launcher, routes input,
lifecycle, and shared service contracts without Cardputer includes in Core, and
reuses that Core from the host SDL preview.

C++ and C are the primary implementation languages. Use Python only for auxiliary
tooling, automation, or host-side verification.

Use Chinese for user-facing explanations when appropriate. Write source code,
identifiers, code comments, commit messages, and diagnostic output in English.

## Hardware and toolchain contract

Treat [platformio.ini](platformio.ini) as the source of truth for the build contract:

- Default environment: `m5stack-cardputer`
- Environment: `m5stack-cardputer`
- Platform: `espressif32@6.7.0`
- Board: `esp32-s3-devkitc-1` (8 MB flash)
- Framework: Arduino
- Language standard: C++17 with `-std=gnu++17`
- Hardware library: `M5Cardputer`
- Serial speed: `115200`
- Upload speed: `1500000`

Keep the USB CDC build flags unless the user explicitly changes the serial transport:

- `ARDUINO_USB_CDC_ON_BOOT=1`
- `ARDUINO_USB_MODE=1`

The project intentionally uses the generic ESP32-S3 DevKitC-1 board definition for
Cardputer ADV. Preserve that choice and the pinned platform version unless a hardware
or dependency migration is explicitly requested.

## Repository layout

- `src/`: firmware entry point and production implementation
- `include/`: project-wide public headers and configuration
- `lib/`: private project libraries
- `test/`: PlatformIO test code
- `tools/sdl-preview/`: host `luma-sdl-preview` CMake target and SDL adapters
- `platformio.ini`: build, upload, dependency, and monitor configuration
- `README.md`: user-facing setup and hardware workflow
- `LICENSE`: MIT License

Keep `main.cpp` small: serial setup, `M5Cardputer.begin`, one hardware update, and
`luma.update()`. Shared behavior lives in `include/luma` and `src/luma`. Cardputer
adapters stay in `src/luma/platform/cardputer`. Host file, clock, audio, and
diagnostics adapters stay in `src/luma/platform/host`. SDL window and keyboard
adapters stay in `tools/sdl-preview`.

## Firmware conventions

Initialize the device through the M5Cardputer API:

```cpp
auto config = M5.config();
M5Cardputer.begin(config, true);
```

Call `M5Cardputer.update()` once per loop before processing input. Route boot and
diagnostic events through `Serial` using stable prefixes such as `[BOOT]`, `[KEY]`,
and `[ERROR]`. Keep the main loop responsive; avoid long blocking delays and place
hardware-independent logic in functions that can be exercised without the device.

## Standard workflow

1. Inspect `platformio.ini`, the relevant source files, and `git status` before editing.
2. Make the smallest change that satisfies the request and keep unrelated working-tree
   changes intact.
3. Build the firmware after source or configuration changes.
4. Report the exact validation result, including any unavailable hardware or tool.

On Windows, when `pio` is not on `PATH`, use the installed PlatformIO executable:

```powershell
$pio = Join-Path $env:USERPROFILE '.platformio\penv\Scripts\pio.exe'
& $pio run
```

Useful commands:

```powershell
& $pio device list
& $pio device monitor
& $pio run --target clean
$env:PATH = "$env:USERPROFILE\.platformio\packages\toolchain-gccmingw32\bin;" + $env:PATH
& $pio test -e native
```

SDL preview (requires CMake, vcpkg, and `VCPKG_ROOT`):

```powershell
cmake -S tools/sdl-preview -B build/sdl-preview `
    -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build/sdl-preview --config Debug --target luma-sdl-preview
```

Hardware upload changes the connected device and requires an explicit user request:

```powershell
& $pio run --target upload
```

If upload cannot connect, use the Cardputer ADV download-mode sequence: power off,
hold `G0`, reconnect the USB-C data cable, release `G0`, and retry.

## Verification standard

For a normal code or configuration change, completion means:

- `pio run` succeeds for `m5stack-cardputer`.
- The changed behavior has a focused verification path, either a PlatformIO test or
  a documented serial/display check.
- No generated `.pio` output or machine-local IDE/Agent state is added to the change.
- The final report names files changed and commands run.

The current firmware's device-level smoke check is:

- Display shows the Boot screen Logo, then Launcher with SETTINGS, ABOUT, and NOTES.
- Serial monitor prints `[BOOT] Luma Cardputer ADV started`.
- App transitions emit `[APP] enter` / `[APP] exit`.
- Typed keys may emit `[KEY]`; they are not echoed on the boot screen.

## Dependency and safety rules

Prefer the existing `M5Cardputer` dependency and PlatformIO configuration. Keep
dependency changes explicit and build them before reporting completion. Keep secrets,
local environment files, build caches, firmware artifacts, and IDE/Agent directories
out of version control according to `.gitignore`.

Default work is host-side inspection, editing, and compilation. Device upload,
serial-session ownership, destructive cleanup, dependency upgrades, and repository
publishing are separate actions that require the user's explicit request.

## Contribution conventions

Use the commit format and pull-request guidance in `CONTRIBUTING.md`. GitHub pull
requests are prefilled from `.github/pull_request_template.md`.
Pull request titles must use the `[Type] Description` format, for example
`[Feat] Establish the Core coordinator`. Use a concise, capitalized type such as
`Feat`, `Fix`, `Docs`, `Refactor`, `Test`, `Chore`, or `CI`.

## Agent skills

### Issue tracker

Issues for this repo live as GitHub issues; use the `gh` CLI. See `docs/agents/issue-tracker.md`.

### Triage labels

Use the default five triage labels: `needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, and `wontfix`. See `docs/agents/triage-labels.md`.

### Domain docs

This is a single-context repo; read root `CONTEXT.md` and `docs/adr/` when they exist. See `docs/agents/domain.md`.
