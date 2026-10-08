# U-Boot 2017.09 for Incari RK3568 OS v1.1.2 (Rockchip RK3568)

This directory tree is the complete U-Boot source that was compiled into the
`uboot.img` of Incari RK3568 OS v1.1.2, published by Incari GmbH. Everything
outside `INCARI/` is the U-Boot source exactly as built. `INCARI/` holds the
patches, the configuration, the build script and this file.

U-Boot is licensed under GPL-2.0-or-later (see `Licenses/README`).

## Source

Base: the Rockchip U-Boot 2017.09 tree mirrored at

- repository: <https://github.com/JeffyCN/mirrors>, branch `u-boot`
- commit `a93658f8f45dc0266be21840931131b10c325e03`
  ("video/drm: dsi2: recalculate vfrefresh after modify mode", 2023-10-08),
  git tree `ac3038d5c630e09ad6823bf566753c84c5bde1d9`

Patches applied on top, in this order (`INCARI/patches/series`, all `-p1`):

| # | Patch | Origin |
|---|-------|--------|
| 1 | `0001-disable-edp-when-init-cpu.patch` | Board-specific change, written by the board supplier and applied by Incari GmbH (change dated 2025-08-11). Writes two eDP PHY GRF registers in `arch_cpu_init()` (`arch/arm/mach-rockchip/rk3568/rk3568.c`) so the unused eDP PHY is off. |
| 2 | `0002-rk3568-build-without-libxbc-and-lib-sha512.patch` | Change made by Incari GmbH for Incari RK3568 OS (dated 2026-10-07). Removes `CONFIG_XBC` and `CONFIG_SHA512` from `configs/rk3568_defconfig` and puts `#ifdef CONFIG_XBC` around `#include <xbc.h>` in `arch/arm/mach-rockchip/board.c`, so `lib/libxbc` and `lib/sha512.c` are no longer compiled. `lib/libxbc` was called only for an Android boot image with header version 4 or later; the code of `lib/sha512.c` was already discarded by the linker. |
| 3 | `meta-rockchip/0001-kbuild-use-fmacro-prefix-map-to-make-__FILE__-a-rela.patch` | [meta-rockchip](https://github.com/JeffyCN/meta-rockchip) `697dcfe9898422540ef611c57ce73d178a6b7e56` (branch kirkstone), `recipes-bsp/u-boot/u-boot-rockchip/` |
| 4 | `meta-rockchip/0002-HACK-Support-python3-for-dtoc.patch` | same |
| 5 | `meta-rockchip/0003-Revert-Makefile-enable-Werror-option.patch` | same |

Patches 1 and 2 come first because the Incari Yocto layer adds them to
`SRC_URI` before meta-rockchip's `auto-patch` class appends its patch
directory. Patches 1 and 2 and the patches from meta-rockchip change
different files, so this order does not change the result.

Notice under GPL-2.0 section 2(a): Incari GmbH distributes U-Boot with the
changes in patches 1 and 2 (dated 2025-08-11, 2026-10-07). The patches from
meta-rockchip carry their own authors and dates.

### Tree-id check

The tree outside `INCARI/` (13,707 files and symlinks) has the git tree id
`425c4ed22721c6e24150e7be48330447d3dfd63b`.

In a clone of this repository, from its top-level directory:

```sh
git ls-tree v1.1.2:u-boot | awk '$4 != "INCARI"' | git mktree
# 425c4ed22721c6e24150e7be48330447d3dfd63b
```

In the `u-boot/` directory of the release tarball (before building in it):

```sh
git init -q && git add -f -A -- . ':(exclude)INCARI' && git write-tree
# 425c4ed22721c6e24150e7be48330447d3dfd63b
```

From the public base commit, with the patches from `INCARI/patches` (set
`P` to the absolute path of that directory):

```sh
git init -q u-boot-base && cd u-boot-base
git fetch -q --depth 1 https://github.com/JeffyCN/mirrors.git a93658f8f45dc0266be21840931131b10c325e03
git checkout -q FETCH_HEAD
while read -r patch opt; do
    patch $opt --no-backup-if-mismatch -s -i "$P/$patch"
done < "$P/series"
git add -f -A && git write-tree
# 425c4ed22721c6e24150e7be48330447d3dfd63b
```

Use `patch --no-backup-if-mismatch` (or `git apply`). Hunk 2 of patch 1
applies with an offset of -6 lines. Without that option GNU patch leaves an
`rk3568.c.orig` backup file behind, which `git add -f -A` would pick up.
`git add` needs `-f` because the upstream `.gitignore` ignores some U-Boot
files (`.checkpatch.conf`, `.travis.yml`, `*.bin`, `*.patch`). For the same
reason, this directory was committed with `git add -f`, and `INCARI/patches/`
would otherwise be ignored too.

## Configuration

- defconfig: `configs/rk3568_defconfig` (board `evb_rk3568`, device tree `rk3568-evb`)
- config fragments: none
- The Yocto build ran `make rk3568_defconfig`, then `yes '' | make oldconfig`.
  The resulting `.config` is in `INCARI/config/rk3568.config`.

## Toolchain

The build used the Yocto Project 4.0 (kirkstone) cross toolchain, from poky
`b210ed67de098a78a577baf464fae569d020de62` (yocto-4.0.25-86):

- `aarch64-oe-linux-gcc (GCC) 11.5.0`
- `GNU ld (GNU Binutils) 2.38.20220708`
- `CC` was `aarch64-oe-linux-gcc --sysroot=<target sysroot>`. The sysroot only
  supplies `libgcc.a` (gcc 11.5.0) for the `-lgcc` in the final link.
- The host tools (`HOSTCC`, including `scripts/dtc`) were built with the host
  gcc. The Yocto command line also gave `HOSTCC` include, library and rpath
  flags for its native sysroot, and set `STAGING_INCDIR`/`STAGING_LIBDIR`.
  They only affect host tools (this tree does not use the `STAGING_*`
  variables), so they are left out here.

The byte-for-byte match described below was checked only with this
toolchain. Other compilers produce different binaries.

## Build

`INCARI/build.sh WORKDIR` runs the same steps as the Yocto recipe (poky
`meta/recipes-bsp/u-boot/u-boot.inc` plus meta-rockchip
`recipes-bsp/u-boot/u-boot-rockchip.bb`). It keeps this tree unchanged and
works on a copy in `WORKDIR/git`. In short:

```sh
export SOURCE_DATE_EPOCH=1696731268 TZ=UTC ARCH=arm64
S=WORKDIR/git     # copy of this tree without INCARI/
B=WORKDIR/build
mk() {
    make -j48 CROSS_COMPILE=aarch64-oe-linux- \
        CC="aarch64-oe-linux-gcc --sysroot=$SYSROOT" V=1 \
        HOSTCC="gcc -O2 -pipe" -C "$S" O="$B" "$@"
}

# do_configure:prepend (meta-rockchip), applied to the copy: python3 shebangs
# and make_fit_atf.py syntax, drop the make/toolchain/clean stages from
# make.sh, PLAT=rk3568 in make.sh (build.sh has the exact sed commands)

mk rk3568_defconfig
yes '' | mk oldconfig
echo > "$B/.scmversion"; echo > "$S/.scmversion"   # no -g<sha> version suffix
mk all
mk u-boot-initial-env

# Rockchip packing (meta-rockchip do_compile:append); rkbin checked out at WORKDIR/rkbin
cd "$B"
for d in make.sh scripts configs arch/arm/mach-rockchip; do cp -rT "$S/$d" "$d"; done
./make.sh                  # -> uboot.img and rk356x_spl_loader_v1.19.113.bin
ln -sf *_loader*.bin loader.bin
./make.sh --idblock        # -> idblock.bin
```

Outputs: `u-boot-nodtb.bin` and `u-boot.dtb` (the U-Boot parts of `uboot.img`),
`uboot.img` and `loader.bin`.

## Rockchip binaries (rkbin), not included here

`uboot.img` and `loader.bin` also contain Rockchip binaries. These are not
part of this source and are not included. They come from the rkbin
repository:

- repository: <https://github.com/JeffyCN/mirrors>, branch `rkbin`
- commit `c41b714cacd249e3ef69b2bbe774da5095eefd72` (2023-10-07), git tree
  `98c2d52a3499757b02f2764b0583b4914923cf72`
- licence: Rockchip's binary licence (`LICENSE` in that repository)
- config files used: `RKTRUST/RK3568TRUST.ini` (FIT contents) and
  `RKBOOT/RK3568MINIALL.ini` (loader contents). The loader was packed with
  `tools/boot_merger` (ver 1.31) from the same commit.

| rkbin file | Version string | SHA-256 | Where it ends up |
|------------|----------------|---------|------------------|
| `bin/rk35/rk3568_bl31_v1.44.elf` | TF-A `v2.3-645-g8cea6ab0b`, fwver v1.44 | `65110f822fdbdd0163ce2dabc60591e7a8a0ffbc9471780e29eef0062f9ed7b6` | `uboot.img` FIT images `atf-1`..`atf-6` (its 6 PT_LOAD segments) |
| `bin/rk35/rk3568_bl32_v2.11.bin` | OP-TEE `3.13.0-743-gb5340fd65` | `6c92ef6a0e6b57ece96861b7fcfb4bc611d12adefecf8115ff45cc14a1bec527` | `uboot.img` FIT image `optee` |
| `bin/rk35/rk3568_ddr_1560MHz_v1.19.bin` | DDR v1.19 | `be35e116bfb3b1b95004e2189b9c2debabd29d2dadd03e632280a4c0085d4df5` | `loader.bin` (471 entry and FlashData) and `idblock.bin` |
| `bin/rk35/rk356x_usbplug_v1.17.bin` | usbplug v1.17 | `4038b7857b840f539760decc0daf1601b8ff61cc17798101e93b11128a7f333e` | `loader.bin` (472 entry, used only for USB download) |
| `bin/rk35/rk356x_spl_v1.13.bin` | `U-Boot SPL 2017.09-ge4e124926e-230922`, fwver v1.13 | `180cb012aa8b3351002abdf16dcd0f59b20629a8d54a5c5b53d7d6621d812ff4` | `loader.bin` (FlashBoot) and `idblock.bin` |

The first-stage SPL is Rockchip's prebuilt binary, not the `spl/u-boot-spl.bin`
built from this tree. Its version string names U-Boot commit
`e4e124926e52084012aea8070c76272fb574b6c4` ("rockchip: spl: Fix fwver
prefix", 2023-09-22). That commit is in the public history of the same
`u-boot` branch, 29 commits before the base commit above.

## Expected results

With the toolchain above and rkbin `c41b714c`:

| File | SHA-256 |
|------|---------|
| `u-boot-nodtb.bin` (FIT image `uboot`, 1,321,304 bytes) | `a53d61e168a36c34b06c6e1efed211e763780409a19f93ca6e05d1c98e86312a` |
| `u-boot.dtb` (FIT image `fdt`, 14,285 bytes) | `036cca588089b26828b70f8c47c7e46068cbfec04a1f6b03c8d8b21dbf6c33ac` |

These are the image hashes stored in the FIT of the shipped `uboot.img`. The
rebuilt `uboot.img` matches the shipped one except for the address fields of
the FDT memory-reservation entries. `make.sh` fills them with host memory
addresses, so they change on every run. The rebuilt `loader.bin` matches
except for its build timestamp and the CRC32 over the file.
