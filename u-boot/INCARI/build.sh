#!/bin/bash
# Rebuild the Incari RK3568 OS v1.1.2 U-Boot the way its Yocto recipe did.
#
# Usage:  INCARI/build.sh WORKDIR
#
# WORKDIR gets the same layout as the Yocto work directory:
#   WORKDIR/git    copy of this source tree (without INCARI/), with the
#                  recipe's configure-time edits applied (step 2)
#   WORKDIR/build  the out-of-tree build directory (O=)
#   WORKDIR/rkbin  symlink to $RKBIN (only when RKBIN is set)
#
# Environment:
#   CROSS_COMPILE  cross tool prefix, must be in PATH (default: aarch64-oe-linux-)
#   SYSROOT        optional target sysroot; the Yocto build used
#                  CC="${CROSS_COMPILE}gcc --sysroot=<target sysroot>", which
#                  is where the linker finds libgcc.a for "-lgcc"
#   HOSTCC         host compiler (default: gcc -O2 -pipe)
#   JOBS           parallel make jobs (default: nproc)
#   RKBIN          optional path to a checkout of the rkbin repository at
#                  commit c41b714cacd249e3ef69b2bbe774da5095eefd72; when set,
#                  uboot.img, the loader and idblock.img are packed as well
#
# The script needs bash, GNU make, GNU sed, python3 and a host gcc.
set -euo pipefail

TOP=$(cd "$(dirname "$0")/.." && pwd)
WORK=${1:?usage: $0 WORKDIR}
mkdir -p "$WORK"
WORK=$(cd "$WORK" && pwd)
S=$WORK/git
B=$WORK/build
CROSS_COMPILE=${CROSS_COMPILE:-aarch64-oe-linux-}
HOSTCC=${HOSTCC:-gcc -O2 -pipe}
JOBS=${JOBS:-$(nproc)}
if [ -n "${SYSROOT:-}" ]; then
	TARGET_CC="${CROSS_COMPILE}gcc --sysroot=${SYSROOT}"
else
	TARGET_CC="${CROSS_COMPILE}gcc"
fi

# Reproducible build settings used by the Yocto build.
export SOURCE_DATE_EPOCH=1696731268
export TZ=UTC
export ARCH=arm64
export PYTHONHASHSEED=0

if [ -e "$S" ] || [ -e "$B" ]; then
	echo "ERROR: $S or $B already exists; use an empty WORKDIR" >&2
	exit 1
fi

# 1. Source copy (Yocto: ${S}), without the INCARI/ directory.
mkdir -p "$S" "$B"
(cd "$TOP" && tar cf - --exclude=./INCARI .) | (cd "$S" && tar xf -)

# 2. do_configure:prepend() of meta-rockchip recipes-bsp/u-boot/u-boot-rockchip.bb
#    (RK_SOC_FAMILY = rk3568, PYTHON_PN = python3). These edits touch only
#    python scripts and make.sh; the last sed matches nothing in this tree.
for s in $(grep -rIl python "$S"); do
	sed -i -e '1s|^#!.*python[23]*|#!/usr/bin/env python3|' "$s"
done
sed -i -e 's/\(open([^,]*\))/\1, "rb")/' \
	-e 's/print >> \([^,]*\), *\(.*\),*$/print(\2, file=\1)/' \
	-e 's/print \(.*\)$/print(\1)/' \
	"$S/arch/arm/mach-rockchip/make_fit_atf.py"
sed -i -e '/^select_tool/d' -e '/^clean/d' -e '/^\t*make/d' -e '/which python2/{n;n;s/exit 1/true/}' "$S/make.sh"
sed -i "s/PLAT=.*/PLAT=rk3568/" "$S/make.sh"
[ ! -e "$S/.config" ] || make -C "$S" mrproper
sed -i 's/ found;/ found = NULL;/' "$S/lib/avb/libavb/avb_slot_verify.c"

mk() {
	make -j "$JOBS" CROSS_COMPILE="$CROSS_COMPILE" CC="$TARGET_CC" V=1 \
		HOSTCC="$HOSTCC" -C "$S" O="$B" "$@"
}

# 3. do_configure (poky meta/recipes-bsp/u-boot/u-boot.inc, UBOOT_MACHINE =
#    rk3568_defconfig, no config fragments) and cml1 "yes '' | make oldconfig".
mk rk3568_defconfig
(cd "$B" && { yes '' || true; } | mk oldconfig)

# 4. do_compile (poky u-boot.inc): empty .scmversion, then "all" and the
#    initial environment.
echo > "$B/.scmversion"
echo > "$S/.scmversion"
mk all
mk u-boot-initial-env

echo "Built: $B/u-boot.bin $B/u-boot-nodtb.bin $B/u-boot.dtb $B/spl/u-boot-spl.bin"

# 5. do_compile:append() of meta-rockchip u-boot-rockchip.bb: pack the
#    Rockchip images with make.sh (needs the rkbin repository at ../rkbin).
if [ -z "${RKBIN:-}" ]; then
	echo "RKBIN not set: skipping uboot.img / loader / idblock packing."
	exit 0
fi
ln -sfn "$(cd "$RKBIN" && pwd)" "$WORK/rkbin"
cd "$B"
for d in make.sh scripts configs arch/arm/mach-rockchip; do
	cp -rT "$S/$d" "$d"
done
./make.sh
ln -sf *_loader*.bin loader.bin
if ls *.img | grep -q idblock; then
	ln -sf *_idblock_*.img idblock.img
else
	./make.sh --idblock
	ln -sf idblock.bin idblock.img
fi
echo "Packed: $B/uboot.img $B/loader.bin $B/idblock.img"
