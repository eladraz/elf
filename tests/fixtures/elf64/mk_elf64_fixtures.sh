#!/bin/sh
#
# Rebuilds the two ELF64 ET_EXEC fixtures used by tools/testElf64Image:
#   aarch64-exec.elf  (EM_AARCH64, R_AARCH64_ABS64 relocations)
#   riscv64-exec.elf  (EM_RISCV, R_RISCV_64 relocations)
#
# Each image is a static, non-PIE link of one assembly file with
# ld.lld --emit-relocs, so the .rela.data section keeps the absolute
# relocations of the .quad entries. Both images have a global and a local
# STT_FUNC symbol, a .data table holding two function addresses and a .bss
# object. Output is deterministic (no build id, fixed inputs).
#
# Needs llvm-mc and ld.lld (LLVM 20). Run from any directory:
#   sh tests/fixtures/elf64/mk_elf64_fixtures.sh

set -eu

dir=$(cd "$(dirname "$0")" && pwd)
tmp=$(mktemp -d "${TMPDIR:-/tmp}/mk_elf64_fixtures.XXXXXX")
trap 'rm -rf "$tmp"' EXIT

for tool in llvm-mc ld.lld; do
	if ! command -v "$tool" >/dev/null 2>&1; then
		echo "mk_elf64_fixtures: $tool not found" >&2
		exit 2
	fi
done

cat > "$tmp/a64.s" <<'EOF'
	.text
	.globl	_start
	.type	_start,@function
_start:
	mov	x8, #93
	mov	w0, #0
	svc	#0
	.size	_start, .-_start
	.type	local_fn,@function
local_fn:
	mov	w0, #1
	ret
	.size	local_fn, .-local_fn
	.data
	.globl	table
	.type	table,@object
	.p2align	3
table:
	.quad	local_fn
	.quad	_start
	.size	table, 16
	.bss
	.globl	counter
	.type	counter,@object
	.p2align	3
counter:
	.zero	8
	.size	counter, 8
EOF

cat > "$tmp/rv64.s" <<'EOF'
	.text
	.globl	_start
	.type	_start,@function
_start:
	li	a7, 93
	li	a0, 0
	ecall
	.size	_start, .-_start
	.type	local_fn,@function
local_fn:
	li	a0, 1
	ret
	.size	local_fn, .-local_fn
	.data
	.globl	table
	.type	table,@object
	.p2align	3
table:
	.quad	local_fn
	.quad	_start
	.size	table, 16
	.bss
	.globl	counter
	.type	counter,@object
	.p2align	3
counter:
	.zero	8
	.size	counter, 8
EOF

llvm-mc -triple=aarch64-linux-gnu -filetype=obj -o "$tmp/a64.o" "$tmp/a64.s"
ld.lld -static --emit-relocs -o "$dir/aarch64-exec.elf" "$tmp/a64.o"

llvm-mc -triple=riscv64-linux-gnu -mattr=+a,+c,+d,+f,+m -filetype=obj -o "$tmp/rv64.o" "$tmp/rv64.s"
ld.lld -static --emit-relocs -o "$dir/riscv64-exec.elf" "$tmp/rv64.o"

echo "mk_elf64_fixtures: wrote $dir/aarch64-exec.elf and $dir/riscv64-exec.elf"
