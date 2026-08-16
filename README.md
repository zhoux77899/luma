# Luma for M5Stack Cardputer ADV

Luma is a statically compiled multi-application firmware for the M5Stack Cardputer ADV.
The current milestone is the Core coordinator: boot reaches Launcher, and Apps share a
platform-independent lifecycle and input frame.

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
