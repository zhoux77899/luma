#!/usr/bin/env python3
"""Bundle luma-sdl-preview and its SDL runtime libraries into a zip."""

from __future__ import annotations

import argparse
import shutil
import stat
import subprocess
import sys
import zipfile
from pathlib import Path

SHARED_SUFFIXES = {".dll", ".dylib", ".so"}

SDL_PREVIEW_NOTES = """# Luma SDL preview

This archive is a host-side preview of the same Apps, not firmware.

## Run

Unzip this archive and run `luma-sdl-preview` (or `luma-sdl-preview.exe`) from
the unzipped directory so preview data lands in `data/`.

The window is 960 x 540 with a 240 x 135 logical canvas.

## Platform notes

- macOS: the binary is unsigned. If Gatekeeper blocks it, open it from Finder
  with Control-click, then Open.
- Linux: this build targets Ubuntu 24.04 glibc. Older distributions may not
  run it.
- Windows: a recent 64-bit Windows 10 or 11 is required.

Arrow keys, Enter, Escape, Backspace/Delete, and printable characters map to
InputFrame.
"""


def is_shared_library(path: Path) -> bool:
    name = path.name.lower()
    if path.suffix.lower() in SHARED_SUFFIXES:
        return True
    return ".so." in name


def is_sdl_runtime(path: Path) -> bool:
    return "sdl2" in path.name.lower() and is_shared_library(path)


def collect_sdl_runtime(installed_root: Path, triplet: str) -> list[Path]:
    installed = installed_root / triplet
    found: list[Path] = []
    for folder in (installed / "bin", installed / "lib"):
        if not folder.is_dir():
            continue
        for path in folder.rglob("*"):
            if path.is_file() and is_sdl_runtime(path):
                found.append(path)
    return found


def staging_has_sdl_runtime(dest_dir: Path) -> bool:
    return any(is_sdl_runtime(path) for path in dest_dir.iterdir())


def executable_needs_sdl_runtime(executable: Path) -> bool:
    if sys.platform == "darwin":
        listing = subprocess.check_output(["otool", "-L", str(executable)], text=True)
        return any("sdl2" in line.lower() for line in listing.splitlines()[1:])
    if sys.platform.startswith("linux"):
        listing = subprocess.check_output(["ldd", str(executable)], text=True)
        return any("sdl2" in line.lower() for line in listing.splitlines())
    return True


def copy_runtime(path: Path, dest_dir: Path) -> Path:
    dest = dest_dir / path.name
    if dest.exists() or dest.is_symlink():
        dest.unlink()
    if path.is_symlink():
        target = path.readlink()
        if target.is_absolute() or ".." in target.parts:
            shutil.copy2(path.resolve(), dest)
        else:
            dest.symlink_to(target)
    else:
        shutil.copy2(path, dest)
    return dest


def copy_beside_executable(executable: Path, dest_dir: Path) -> None:
    for path in executable.parent.iterdir():
        if path == executable or not is_sdl_runtime(path):
            continue
        copy_runtime(path, dest_dir)


def make_executable(path: Path) -> None:
    mode = path.stat().st_mode
    path.chmod(mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)


def fix_macos_install_names(executable: Path, dest_dir: Path) -> None:
    listing = subprocess.check_output(["otool", "-L", str(executable)], text=True)
    for raw_line in listing.splitlines()[1:]:
        dep = raw_line.strip().split(" ", 1)[0]
        name = Path(dep).name
        if not name.lower().startswith("libsdl2"):
            continue
        if not (dest_dir / name).exists():
            continue
        subprocess.check_call(
            ["install_name_tool", "-change", dep, f"@executable_path/{name}", str(executable)]
        )

    for lib in dest_dir.iterdir():
        if not is_sdl_runtime(lib) or lib.is_symlink():
            continue
        subprocess.check_call(
            ["install_name_tool", "-id", f"@executable_path/{lib.name}", str(lib)]
        )


def fix_linux_rpath(executable: Path) -> None:
    patchelf = shutil.which("patchelf")
    if patchelf is None:
        return
    subprocess.check_call([patchelf, "--set-rpath", "$ORIGIN", str(executable)])


def write_zip(source_dir: Path, zip_path: Path) -> None:
    with zipfile.ZipFile(zip_path, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        for path in sorted(source_dir.rglob("*")):
            archive.write(path, path.relative_to(source_dir).as_posix())


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", required=True, type=Path)
    parser.add_argument("--triplet", required=True)
    parser.add_argument("--installed-root", required=True, type=Path)
    parser.add_argument("--slug", required=True)
    parser.add_argument("--platform", required=True)
    parser.add_argument("--output-dir", required=True, type=Path)
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    executable = args.executable.resolve()
    if not executable.is_file():
        print(f"executable not found: {executable}", file=sys.stderr)
        return 1

    output_dir = args.output_dir.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    staging_dir = output_dir / f"staging-{args.platform}"
    if staging_dir.exists():
        shutil.rmtree(staging_dir)
    staging_dir.mkdir(parents=True)

    dest_exe = staging_dir / executable.name
    shutil.copy2(executable, dest_exe)
    if dest_exe.suffix.lower() != ".exe":
        make_executable(dest_exe)

    copy_beside_executable(executable, staging_dir)
    for runtime in collect_sdl_runtime(args.installed_root.resolve(), args.triplet):
        copy_runtime(runtime, staging_dir)

    if not staging_has_sdl_runtime(staging_dir) and executable_needs_sdl_runtime(dest_exe):
        print(
            f"SDL runtime not found beside {executable} or under "
            f"{args.installed_root.resolve() / args.triplet}",
            file=sys.stderr,
        )
        return 1

    if sys.platform == "darwin":
        fix_macos_install_names(dest_exe, staging_dir)
    elif sys.platform.startswith("linux"):
        fix_linux_rpath(dest_exe)

    (staging_dir / "SDL-PREVIEW.md").write_text(SDL_PREVIEW_NOTES, encoding="utf-8")

    zip_name = f"luma-sdl-preview-{args.slug}-{args.platform}.zip"
    zip_path = output_dir / zip_name
    if zip_path.exists():
        zip_path.unlink()
    write_zip(staging_dir, zip_path)
    shutil.rmtree(staging_dir)

    if not zip_path.is_file() or zip_path.stat().st_size == 0:
        print(f"failed to write {zip_path}", file=sys.stderr)
        return 1

    print(zip_path.name)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
