#!/usr/bin/env bash
set -euo pipefail

: "${ACTIONS_REFERENCE_ROOT:?Set ACTIONS_REFERENCE_ROOT to the pinned lv_port_actions_technology clone}"
: "${ACTIONS_TOOLCHAIN_ROOT:?Set ACTIONS_TOOLCHAIN_ROOT to Zephyr SDK/toolchain 0.13.2}"

EXPECTED_COMMIT="26f51e584940d5b141cd1d09d2629d69b595f579"
BUILD_DIR="${VENDOR_BUILD_DIR:-/tmp/cmf-vendor-build}"
WEST_BIN="${WEST_BIN:-west}"
ACTUAL_COMMIT="$(git -C "${ACTIONS_REFERENCE_ROOT}" rev-parse HEAD)"

if [[ "${ACTUAL_COMMIT}" != "${EXPECTED_COMMIT}" ]]; then
	echo "reference commit mismatch: expected ${EXPECTED_COMMIT}, got ${ACTUAL_COMMIT}" >&2
	exit 1
fi

export ZEPHYR_BASE="${ACTIONS_REFERENCE_ROOT}/action_technology_sdk/zephyr"
export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
export ZEPHYR_SDK_INSTALL_DIR="${ACTIONS_TOOLCHAIN_ROOT}"

"${WEST_BIN}" build -p auto \
	-b ats3089c_dev_watch \
	-d "${BUILD_DIR}" \
	"${ACTIONS_REFERENCE_ROOT}/hello_world"

test -f "${BUILD_DIR}/zephyr/zephyr.elf"
echo "compile-only vendor reference passed: ${BUILD_DIR}/zephyr/zephyr.elf"
echo "No firmware packing or flashing command was run."
