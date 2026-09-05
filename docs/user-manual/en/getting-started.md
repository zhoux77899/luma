# Getting started

Flash a LUMA release onto a M5Stack Cardputer ADV, then wait for the Boot screen.

You need Python 3.11, `esptool`, a Cardputer ADV, and a data-capable USB-C cable. These images are for that board only. Do not flash them to another device.

If `python` is not 3.11, use `py -3.11` on Windows or `python3.11` on macOS and Linux.

```bash
python -m pip install esptool
```

## Download a release

Release images are on the [GitHub Releases page](https://github.com/zhoux77899/luma/releases). Download the merged image and `SHA256SUMS`.

The merged image is the complete 8 MB flash image. The release Flash package has the split images, addresses in `flash.json`, `FLASHING.md`, and checksums. This page uses the merged image.

## Verify the download

Compare the hash with the matching line in `SHA256SUMS`.

Windows PowerShell:

```powershell
Get-FileHash .\luma-cardputer-vX.Y.Z-merged.bin -Algorithm SHA256
```

macOS:

```bash
shasum -a 256 luma-cardputer-vX.Y.Z-merged.bin
```

Linux:

```bash
sha256sum --ignore-missing -c SHA256SUMS
```

## Enter download mode

Power off the Cardputer ADV, hold **G0** while reconnecting a data-capable USB-C cable, then release **G0**.

## Flash the merged image

Write the image at address `0x00000000`:

```bash
esptool.py --chip esp32s3 write_flash 0x00000000 luma-cardputer-vX.Y.Z-merged.bin
```

If `esptool.py` is not on `PATH`, use `py -3.11 -m esptool` on Windows or `python3.11 -m esptool` on macOS and Linux. If automatic port detection fails, add `--port <PORT>` (for example `COM5`, `/dev/cu.usbmodem*`, or `/dev/ttyUSB0`).

## Boot screen

Power the Cardputer ADV. LUMA shows the Boot screen first: a short Logo splash. The Boot screen is not an App. It does not take keys.

Launcher opens next. If the screen stays black, repeat download mode and flash.

Next: [Keys](keys.md), then [Launcher](launcher.md).
