# Luma for M5Stack Cardputer ADV

This project is a minimal PlatformIO + Arduino firmware for the M5Stack Cardputer ADV.
The first milestone verifies the display, keyboard, and serial monitor before the Luma
system, shell, SDK, and applications are added.

## Build

PlatformIO Core is installed in the user environment on Windows. If `pio` is not on
PATH, run the bundled executable directly:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run
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

The initial firmware prints `[BOOT]` and `[KEY]` messages and echoes typed characters
to the display.
