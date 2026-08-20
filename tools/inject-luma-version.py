Import("env")

import os
import re
import subprocess


def version_already_set():
    pio_flags = os.environ.get("PLATFORMIO_BUILD_FLAGS", "")
    if "LUMA_VERSION" in pio_flags:
        return True
    flags = env.GetProjectOption("build_flags")
    if isinstance(flags, str):
        flags = [flags]
    for flag in flags or []:
        if "LUMA_VERSION" in str(flag):
            return True
    for item in env.get("CPPDEFINES", []):
        name = item[0] if isinstance(item, (list, tuple)) else item
        if name == "LUMA_VERSION":
            return True
    return False


def base_version():
    path = os.path.join(env["PROJECT_DIR"], "include", "luma", "version.h")
    with open(path, encoding="utf-8") as handle:
        text = handle.read()
    match = re.search(r'#define LUMA_VERSION "([^"]+)"', text)
    return match.group(1) if match else "0.1.0"


def short_sha():
    try:
        output = subprocess.check_output(
            ["git", "rev-parse", "--short=7", "HEAD"],
            cwd=env["PROJECT_DIR"],
            stderr=subprocess.DEVNULL,
        )
        sha = output.decode().strip()
        return sha if sha else None
    except (OSError, subprocess.CalledProcessError):
        return None


if not version_already_set():
    sha = short_sha()
    identity = "{}.{}".format(base_version(), sha if sha else "dev")
    env.Append(CPPDEFINES=[("LUMA_VERSION", env.StringifyMacro(identity))])
