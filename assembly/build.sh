#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

assemble() {
	local src="$1"
	local bin="$2"
	local lst="$3"

	z80asm -l -o "$bin" "$src" 2> >(tee "$lst" >&2)
}

assemble BIOS.ASM BIOS.BIN BIOS.LST
assemble BOOT2STAGE.ASM BOOT2STAGE.BIN BOOT2STAGE.LST
assemble BOOTLOADER.ASM BOOTLOADER.BIN BOOTLOADER.LST