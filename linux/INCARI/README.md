# Incari RK3568 OS v1.1.2: Linux kernel source

This tree is the complete corresponding source of the Linux kernel shipped in Incari RK3568 OS
v1.1.2 by Incari GmbH: the kernel image and the device tree blob. The kernel modules are also built
from this tree, but the v1.1.2 images do not install them (see "Kernel modules" below).

- Linux 5.10.198: the Rockchip RK3568 BSP kernel as supplied by the board vendor, plus changes made
  by Incari GmbH (listed below). Kernel release string: `5.10.198-rockchip-standard`.
- Licence: GPL-2.0, see `COPYING` and `LICENSES/`. Individual files carry their own SPDX identifiers
  and copyright notices.

## This directory

| File | What it is |
|------|------------|
| `kernel.config` | The exact `.config` of the shipped kernel. It is byte-identical to the configuration embedded in the shipped kernel (`CONFIG_IKCONFIG`, `/proc/config.gz`). |
| `README.md` | This file. |

## Shipped binaries (SHA-256)

| Binary | SHA-256 |
|--------|---------|
| `arch/arm64/boot/Image` | `ecb37d564df2b02f96fae375cc7015a31d511e46c551098d6ae3880d81dc66fa` |
| `arch/arm64/boot/Image.lz4` (the kernel inside the boot partition) | `22f2f0835bebb5e115e7f9b289cce2e4ac3cd8dbbd43c3972f0131e0d16bc13f` |
| `arch/arm64/boot/dts/rockchip/ntimes-linux-rk3568-k510-v2.dtb` (`rk-kernel.dtb` in `resource.img`) | `3cb8df971077ae94c7973e939c90bb66e5d2f21e862dd5d50eeb8d2df4bebe69` |

The boot partition is `zboot.img`: an Android boot image (header v0, page size 2048) with
kernel = `Image.lz4` and second stage = `resource.img` (entries `rk-kernel.dtb`, `logo.bmp`,
`logo_kernel.bmp`).
The recovery partition (`recovery.img`, a FIT image) carries the same `Image` and DTB.

## Toolchain and build environment

| Item | Value |
|------|-------|
| Build system | Yocto Project 4.0 "Kirkstone" (poky `b210ed67de098a78a577baf464fae569d020de62`, kirkstone branch) |
| Cross compiler | GCC 11.5.0 (`gcc-cross` of that poky), target triplet `aarch64-oe-linux` |
| Binutils | GNU Binutils 2.38.20220708 (`ld.bfd`) |
| `ARCH` | `arm64` |
| `CROSS_COMPILE` | `aarch64-oe-linux-` |
| `CC` | `aarch64-oe-linux-gcc --sysroot=<aarch64 glibc sysroot>` |
| `LD` | `aarch64-oe-linux-ld.bfd --sysroot=<aarch64 glibc sysroot>` |
| `KBUILD_BUILD_USER` | `oe-user` |
| `KBUILD_BUILD_HOST` | `oe-host` |
| `KBUILD_BUILD_VERSION` | `1` |
| `KBUILD_BUILD_TIMESTAMP` | `Fri Apr 25 09:46:33 UTC 2025` |
| `SOURCE_DATE_EPOCH` | `1745574393` |
| `KCONFIG_NOTIMESTAMP` | `1` |
| Build host | x86_64 Linux (Ubuntu 22.04), host GCC 11.4, GNU gzip 1.10, lz4 1.9.4, Python 3, bison, flex, bc, OpenSSL headers |

Other aarch64 Linux cross compilers should also build this tree, but only the toolchain above was
tested. Bit-identical output needs the versions above (see "Reproducing the shipped binaries").

## Build

```sh
export ARCH=arm64 CROSS_COMPILE=aarch64-oe-linux-
export KBUILD_BUILD_USER=oe-user KBUILD_BUILD_HOST=oe-host KBUILD_BUILD_VERSION=1
export KBUILD_BUILD_TIMESTAMP="Fri Apr 25 09:46:33 UTC 2025"
export SOURCE_DATE_EPOCH=1745574393 KCONFIG_NOTIMESTAMP=1
SYSROOT=/path/to/aarch64/glibc/sysroot     # see "Configuration" below

mkdir -p ../build
cp INCARI/kernel.config ../build/.config

make O=../build "CC=aarch64-oe-linux-gcc --sysroot=$SYSROOT" "LD=aarch64-oe-linux-ld.bfd --sysroot=$SYSROOT" \
     -j"$(nproc)" ntimes-linux-rk3568-k510-v2.img
make O=../build "CC=aarch64-oe-linux-gcc --sysroot=$SYSROOT" "LD=aarch64-oe-linux-ld.bfd --sysroot=$SYSROOT" \
     -j"$(nproc)" rockchip/ntimes-linux-rk3568-k510-v2.dtb
make O=../build "CC=aarch64-oe-linux-gcc --sysroot=$SYSROOT" "LD=aarch64-oe-linux-ld.bfd --sysroot=$SYSROOT" \
     -j"$(nproc)" modules
make O=../build "CC=aarch64-oe-linux-gcc --sysroot=$SYSROOT" "LD=aarch64-oe-linux-ld.bfd --sysroot=$SYSROOT" \
     INSTALL_MOD_PATH="$PWD/../modules-install" modules_install
```

The first three are the make targets of the original build, in the same order. The original
build then ran `modules_install` (with `DEPMOD=echo`) into its packaging staging directory. The
modules were packaged, but no v1.1.2 image installs them (see "Kernel modules" below).
`ntimes-linux-rk3568-k510-v2.img` is a Rockchip target (`arch/arm64/Makefile`). It builds
`rockchip/ntimes-linux-rk3568-k510-v2.dtb` and `Image.lz4` (and so `Image`), then runs
`scripts/mkimg --dtb ntimes-linux-rk3568-k510-v2.dtb`, which writes `resource.img`, `boot.img`
(`Image` + `resource.img`) and `zboot.img` (`Image.lz4` + `resource.img`) into the build directory.

Outputs:

- `../build/arch/arm64/boot/Image` and `Image.lz4`
- `../build/arch/arm64/boot/dts/rockchip/ntimes-linux-rk3568-k510-v2.dtb`
- `../build/resource.img`, `../build/boot.img`, `../build/zboot.img`

Notes:

- `scripts/bmpconvert`, `scripts/mkmultidtb.py` and a few other scripts start with
  `#!/usr/bin/env python`. The original build rewrote these shebangs to `python3`. On a host without
  a `python` command, do the same or provide `python` as Python 3.
- If you build from a git checkout of this tree, first create a `.scmversion` file in the top-level
  directory of the kernel tree that contains a single newline (`printf '\n' > .scmversion`). Otherwise `CONFIG_LOCALVERSION_AUTO` appends a git suffix to the
  release string, and the modules directory name no longer matches `5.10.198-rockchip-standard`.
  The original build did the same.

## Configuration

`INCARI/kernel.config` is the configuration that was used. It can also be regenerated from the
in-tree defconfig:

```sh
make O=../build "CC=aarch64-oe-linux-gcc --sysroot=$SYSROOT" rockchip_linux_defconfig
scripts/config --file ../build/.config --set-str LOCALVERSION "-rockchip-standard"
make O=../build "CC=aarch64-oe-linux-gcc --sysroot=$SYSROOT" olddefconfig
```

With the toolchain above, the result is byte-identical to `kernel.config`.

`CC` must be able to link aarch64 user-space programs, which is why `--sysroot` points at an aarch64
glibc sysroot. Kconfig tests this: `kernel.config` has `CONFIG_CC_CAN_LINK=y` and
`CONFIG_CC_CAN_LINK_STATIC=y`. With a compiler that cannot link, Kconfig silently drops those two
options, and the embedded configuration (and so `Image`) differs.

## Boot logos: placeholders

`logo.bmp` and `logo_kernel.bmp` in the top-level directory are neutral placeholders: 480 x 480,
32-bit BMP, every pixel black. They have the same format and file size as the product boot logos
they replace. The product boot-logo artwork is not part of this release.

These files are not compiled into the kernel (`CONFIG_LOGO` is not set). `scripts/mkimg` only packs
them into `resource.img`, next to the DTB, for the boot loader to display. So `Image`, `Image.lz4`
and the DTB do not depend on them. `resource.img`, `boot.img` and `zboot.img` built from this tree
differ from the shipped ones in the two logo entries and their checksums in the `resource.img`
header. In addition, `scripts/resource_tool` leaves a few uninitialised bytes in its entry
headers, so even two builds from identical inputs can differ there.

`scripts/mkimg` calls `scripts/bmpconvert`, which rewrites both files in place in the source tree
(converted to top-down 24-bit). This is expected.

## Reproducing the shipped binaries

Incari GmbH rebuilt this tree outside Yocto with the toolchain and settings above. `Image`,
`Image.lz4` and the DTB came out bit-identical to the shipped ones (SHA-256 above), and `vmlinux`
came out identical to the original build's. Three details matter for bit-identical output:

1. **gzip.** `kernel/config_data.gz` (`CONFIG_IKCONFIG`) is made with `gzip -n -9`. The original
   build used GNU gzip 1.10. Another implementation (for example pigz installed as `gzip`) compresses
   differently. The data then has a different size, and about 200 symbols after it move.
2. **Mali-400 build date.** `drivers/gpu/arm/mali400/mali/Kbuild` writes the output of `date` into the
   generated file `drivers/gpu/arm/mali400/mali/__malidrv_build_info.c`, which is compiled into the
   kernel. The make rule has no prerequisites, so an existing file is kept. To get the shipped
   string, create that file before building, as one line:

   ```c
   const char *__malidrv_build_info(void) { return "malidrv: API_VERSION=900 REPO_URL= REVISION= CHANGED_REVISION= CHANGE_DATE= BUILD_DATE=Fri Sep 25 15:20:20 UTC 2026 BUILD=release TARGET_PLATFORM= MALI_PLATFORM=rk KDIR= OS_MEMORY_KERNEL_BUFFER_SIZE_IN_MB=16 USING_UMP= USING_PROFILING=y USING_INTERNAL_PROFILING= USING_GPU_UTILIZATION=1 USING_DVFS= USING_DMA_BUF_FENCE =  MALI_UPPER_HALF_SCHEDULING=1";}
   ```

3. **GNU build-IDs.** `vmlinux` and the vDSO each carry a 20-byte build-ID. The linker computes it
   over the whole ELF file, including the DWARF debug information (`CONFIG_DEBUG_INFO=y`), and that
   debug information contains the absolute source and build paths. The original build rewrote them
   with `-fdebug-prefix-map`: the source tree to `/usr/src/kernel`, the toolchain sysroots to an empty
   prefix, and the build directory to a path inside its Yocto work directory. When the build runs in
   other directories, `Image` differs from the shipped one only in these 2 x 20 bytes. With the same
   mapping, the rebuild matched every byte.

## Kernel modules

The original build also ran `make modules` in the same build directory. The kernel modules are
built from this tree, but the v1.1.2 images do not install them: their root file system has no
`/lib/modules` directory and no kernel module package. With the same settings and path mapping as
above, the rebuild produced all 155 `.ko` files, `modules.order`, `modules.builtin` and
`modules.builtin.modinfo` byte-identical to those of the original build.

## Changes relative to the vendor BSP kernel

The files below were changed by Incari GmbH relative to the RK3568 BSP kernel as delivered by the
board vendor. This list is the notice required by GPL-2.0 section 2(a). Dates are the dates of the
changes.

| File | Change | Date(s) |
|------|--------|---------|
| `arch/arm64/boot/dts/rockchip/ntimes-np3568.dtsi` | NPU regulator PWM, suspend, MIPI-DSI panel init sequence and orientation, PWM3, USB OTG/host enabled | 2024-07-18, 2024-09-02, 2024-09-16, 2025-02-04 |
| `arch/arm64/boot/dts/rockchip/ntimes-rk3568-evb.dtsi` | GPIO/PWM control, wake-up GPIO, I2C sensors (QMC5883P compass, QMI8658 IMU), sensor pins | 2024-09-02, 2025-03-12 |
| `arch/arm64/boot/dts/rockchip/ntimes-single-lvds.dts` | LVDS bus format (RGB888) | 2024-07-05 |
| `arch/arm64/configs/rockchip_linux_defconfig` | Kernel configuration | 2025-03-12, 2025-04-25 |
| `drivers/gpio/gpio-mphone.c` | Backlight and PWM control, system suspend, MCU reset control | 2024-09-02, 2024-09-13 |
| `drivers/input/sensors/accel/Kconfig`, `Makefile` | QMI8658 accelerometer | 2025-03-12 |
| `drivers/input/sensors/accel/qmi8658_acc.c` | New: QMI8658 accelerometer driver | 2025-03-12 |
| `drivers/input/sensors/accel/mpu6500_acc.c` | IMU data fix (misc device interface) | 2024-10-22 |
| `drivers/input/sensors/compass/Kconfig`, `Makefile` | QMC5883P compass | 2025-03-12 |
| `drivers/input/sensors/compass/qmc5883p.c` | New: QMC5883P compass driver | 2025-03-12 |
| `drivers/input/sensors/compass/ak09918.c` | Continuous mode, bug fix | 2024-09-20, 2024-10-22 |
| `drivers/input/sensors/gyro/Kconfig`, `Makefile` | QMI8658 gyroscope | 2025-03-12 |
| `drivers/input/sensors/gyro/qmi8658_gyro.c` | New: QMI8658 gyroscope driver | 2025-03-12 |
| `drivers/input/sensors/gyro/mpu6500_gyro.c` | IMU data fix (misc device interface) | 2024-10-22 |
| `include/linux/qmi8658.h` | New: QMI8658 definitions | 2025-03-12 |
| `include/linux/sensor-dev.h` | Sensor IDs for QMC5883P and QMI8658 | 2025-03-12 |
| `drivers/input/touchscreen/focaltech_touch/focaltech_core.c` | Touch panel fixes | 2024-07-04 |
| `drivers/input/touchscreen/focaltech_touch/include/firmware/fw_sample.h` | Touch controller firmware image (FocalTech) | 2024-09-16 |
| `drivers/net/wireless/rockchip_wlan/rtl8821cs/` (`Makefile`, `core/efuse/rtw_efuse.c`, `core/rtw_recv.c`, `hal/btc/mp_precomp.h`, `hal/phydm/halrf/rtl8821c/halrf_8821c.c`, `hal/phydm/halrf/rtl8821c/halrf_iqk_8821c.c`) | RTL8821CS Wi-Fi driver fixes | 2024-07-04 |
| `drivers/phy/rockchip/phy-rockchip-inno-usb2.c` | USB host/device mode option, dual USB channel | 2024-07-18, 2025-02-04 |
| `logo.bmp`, `logo_kernel.bmp` | Boot logo images (replaced by placeholders in this release, see above) | 2024-07-04, 2024-09-11 |
