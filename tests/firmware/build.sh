#!/bin/sh
set -eu
if [ "$#" -lt 2 ]; then
    echo "Usage: $0 /absolute/path/to/zmk/app /absolute/path/to/build-directory [extra CMake options]" >&2
    exit 2
fi
task_zmk_app=$1
task_build_dir=$2
shift 2
module_root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
west build -s "$task_zmk_app" -d "$task_build_dir" -b seeeduino_xiao_ble -- \
    -DSHIELD=settings_reset \
    -DZMK_CONFIG="$module_root/tests/firmware" \
    -DKEYMAP_FILE="$module_root/tests/firmware/smoke.keymap" \
    -DEXTRA_CONF_FILE="$module_root/tests/firmware/smoke.conf" \
    -DZMK_EXTRA_MODULES="$module_root" \
    "$@"
