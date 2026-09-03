# 快速开始

把一份 LUMA 发行版刷进 M5Stack Cardputer ADV，然后等 Boot screen（开机画面）。

你需要 Python 3.11、`esptool`、一台 Cardputer ADV，以及一根能传数据的 USB-C 线。这些镜像只针对这块板，不要刷到别的设备上。

如果 `python` 不是 3.11，Windows 用 `py -3.11`，macOS 和 Linux 用 `python3.11`。

```bash
python -m pip install esptool
```

## 下载发行版

发行版镜像在 [GitHub Releases](https://github.com/zhoux77899/luma/releases)。下载 merged image（完整镜像）和 `SHA256SUMS`。

merged image 是完整的 8 MB 闪存镜像。发行版里的 Flash package 含分段镜像、`flash.json` 里的地址、`FLASHING.md` 和校验和。本页只用 merged image。

## 校验下载

把算出来的哈希和 `SHA256SUMS` 里对应的那一行比一下。

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

## 进入下载模式

关掉 Cardputer ADV，按住 **G0**，再插上能传数据的 USB-C 线，然后松开 **G0**。

## 写入 merged image

镜像必须写到地址 `0x00000000`：

```bash
esptool.py --chip esp32s3 write_flash 0x00000000 luma-cardputer-vX.Y.Z-merged.bin
```

如果 `PATH` 上没有 `esptool.py`，Windows 用 `py -3.11 -m esptool`，macOS 和 Linux 用 `python3.11 -m esptool`。自动识别端口失败时加上 `--port <PORT>`（例如 `COM5`、`/dev/cu.usbmodem*` 或 `/dev/ttyUSB0`）。

## Boot screen

给 Cardputer ADV 上电。LUMA 先显示 Boot screen：一小段 Logo。Boot screen 不是 App，也不接收按键。

随后进入 Launcher。如果屏幕一直黑，重新走一遍下载模式再刷。

接下来：[按键](keys.md)，然后 [Launcher](launcher.md)。
