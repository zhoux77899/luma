#!/usr/bin/env bash
# Package Cardputer split flash images, a merged 8 MB image, and checksums.
set -euo pipefail

usage() {
  echo "Usage: package-cardputer-flash.sh --build-dir DIR --identity VERSION --slug SLUG --output-dir DIR" >&2
  exit 2
}

BUILD_DIR=""
IDENTITY=""
SLUG=""
OUTPUT_DIR=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --build-dir)
      BUILD_DIR="$2"
      shift 2
      ;;
    --identity)
      IDENTITY="$2"
      shift 2
      ;;
    --slug)
      SLUG="$2"
      shift 2
      ;;
    --output-dir)
      OUTPUT_DIR="$2"
      shift 2
      ;;
    *)
      usage
      ;;
  esac
done

if [[ -z "${BUILD_DIR}" || -z "${IDENTITY}" || -z "${SLUG}" || -z "${OUTPUT_DIR}" ]]; then
  usage
fi

BUILD_DIR="$(cd "${BUILD_DIR}" && pwd)"
mkdir -p "${OUTPUT_DIR}"
OUTPUT_DIR="$(cd "${OUTPUT_DIR}" && pwd)"

staging_dir="$(mktemp -d "${TMPDIR:-/tmp}/luma-cardputer-flash.XXXXXX")"
cleanup() {
  rm -rf "${staging_dir}"
}
trap cleanup EXIT

merged_name="luma-cardputer-${SLUG}-merged.bin"
archive_name="luma-cardputer-${SLUG}-flash.zip"

for image in bootloader.bin partitions.bin firmware.bin littlefs.bin; do
  source="${BUILD_DIR}/${image}"
  test -s "${source}"
  cp "${source}" "${staging_dir}/${image}"
done

pio pkg exec -- esptool.py \
  --chip esp32s3 \
  merge_bin \
  --output "${staging_dir}/${merged_name}" \
  --flash_size 8MB \
  --fill-flash-size 8MB \
  0x00000000 "${staging_dir}/bootloader.bin" \
  0x00008000 "${staging_dir}/partitions.bin" \
  0x00010000 "${staging_dir}/firmware.bin" \
  0x00340000 "${staging_dir}/littlefs.bin"

test "$(stat -c '%s' "${staging_dir}/${merged_name}")" -eq 8388608

export STAGING_DIR="${staging_dir}"
export FLASH_SLUG="${SLUG}"
export FLASH_IDENTITY="${IDENTITY}"
export MERGED_NAME="${merged_name}"

python - <<'PY'
import json
import os
from pathlib import Path

staging_dir = Path(os.environ["STAGING_DIR"])
slug = os.environ["FLASH_SLUG"]
identity = os.environ["FLASH_IDENTITY"]
merged_name = os.environ["MERGED_NAME"]

manifest = {
    "tag": slug,
    "version": identity,
    "hardware": "M5Stack Cardputer ADV",
    "chip": "ESP32-S3",
    "flash_size": "8MB",
    "images": [
        {"file": "bootloader.bin", "address": "0x00000000"},
        {"file": "partitions.bin", "address": "0x00008000"},
        {"file": "firmware.bin", "address": "0x00010000"},
        {"file": "littlefs.bin", "address": "0x00340000"},
    ],
    "merged_image": {
        "file": merged_name,
        "address": "0x00000000",
    },
}

(staging_dir / "flash.json").write_text(
    json.dumps(manifest, indent=2) + "\n",
    encoding="utf-8",
)
PY

cat > "${staging_dir}/FLASHING.md" <<EOF
# Luma ${SLUG} Cardputer ADV firmware

The merged image is an 8 MB ESP32-S3 flash image. Write it at address \`0x00000000\`:

\`\`\`bash
esptool.py --chip esp32s3 write_flash 0x00000000 ${merged_name}
\`\`\`

The split images and their addresses are recorded in \`flash.json\`. The package checksum file covers every split image and the merged image.
EOF

(
  cd "${staging_dir}"
  sha256sum bootloader.bin partitions.bin firmware.bin littlefs.bin "${merged_name}" > SHA256SUMS
  sha256sum --check SHA256SUMS
)

rm -rf "${OUTPUT_DIR:?}/"*
cp "${staging_dir}/${merged_name}" "${OUTPUT_DIR}/${merged_name}"
(
  cd "${staging_dir}"
  zip -q -j "${OUTPUT_DIR}/${archive_name}" ./*
)

unzip -t "${OUTPUT_DIR}/${archive_name}"
(
  cd "${OUTPUT_DIR}"
  sha256sum "${archive_name}" "${merged_name}" > SHA256SUMS
  sha256sum --check SHA256SUMS
)

test -s "${OUTPUT_DIR}/${archive_name}"
test -s "${OUTPUT_DIR}/${merged_name}"
test -s "${OUTPUT_DIR}/SHA256SUMS"
find "${OUTPUT_DIR}" -maxdepth 1 -type f -printf '%f\n' | sort
