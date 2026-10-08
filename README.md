# Incari RK3568 OS: open-source components

This repository is published by Incari GmbH. It holds the source code of the open-source
Linux kernel and U-Boot boot loader shipped in Incari RK3568 OS, an embedded Linux operating
system for a Rockchip RK3568 device, built with the Yocto Project 4.0 (Kirkstone).
It also holds the changed files of other components that open-source packages of the OS are
compiled with: the Khronos EGL header `include/EGL/eglext.h` of Rockchip's Mali driver package
(libmali), in [`rockchip-libmali/`](rockchip-libmali/).

This is release **v1.1.2**. Release tags match OS releases: tag `v1.1.2` holds the sources of
Incari RK3568 OS v1.1.2.

## Components in this repository

| Directory | Component | Version | Licence |
|-----------|-----------|---------|---------|
| [`linux/`](linux/) | Linux kernel: the Rockchip RK3568 BSP kernel from the board vendor, with changes by Incari GmbH | 5.10.198 (release string `5.10.198-rockchip-standard`) | GPL-2.0-only, see [`linux/COPYING`](linux/COPYING) and [`linux/LICENSES/`](linux/LICENSES/) |
| [`u-boot/`](u-boot/) | U-Boot boot loader: Rockchip U-Boot 2017.09 with five patches | 2017.09 (base commit `a93658f8f45dc0266be21840931131b10c325e03` of <https://github.com/JeffyCN/mirrors>, branch `u-boot`) | GPL-2.0-or-later, see [`u-boot/Licenses/README`](u-boot/Licenses/README) |
| [`rockchip-libmali/`](rockchip-libmali/) | Khronos EGL header `include/EGL/eglext.h` of Rockchip's Mali user-space driver package (libmali), with the change made by Incari GmbH | libmali commit `af1d910c` of <https://github.com/JeffyCN/mirrors>, branch `libmali`, + one patch | Khronos MIT-style licence in the file's header |

Individual files carry their own SPDX identifiers and copyright notices. Third-party notices
are kept unchanged.

`linux/` and `u-boot/` each hold the complete source tree as it was compiled for the release. The only
additions are the `INCARI/` directories, which hold the configuration, the build steps and the
checksums of the shipped binaries. The only replaced files are the two kernel boot logo images
(`linux/logo.bmp`, `linux/logo_kernel.bmp`), which are neutral placeholders here. They are not
compiled into the kernel; see [`linux/INCARI/README.md`](linux/INCARI/README.md).

`rockchip-libmali/` holds what Incari GmbH changed in other components whose files are
compiled into open-source packages of the OS: the changed files exactly as they were compiled, with the
patches and a README in `INCARI/` (the public base commit, the packages that compile the files, their
SHA-256).

## Where the rest of the OS comes from

The other components of the OS are built with the Yocto Project, using these public layers at
these revisions (all on their `kirkstone` branches):

| Layer | Repository | Commit |
|-------|------------|--------|
| poky (OpenEmbedded-Core, BitBake, Yocto Project 4.0) | <https://git.yoctoproject.org/poky> (mirror: <https://github.com/yoctoproject/poky>) | `b210ed67de098a78a577baf464fae569d020de62` |
| meta-openembedded | <https://git.openembedded.org/meta-openembedded> (mirror: <https://github.com/openembedded/meta-openembedded>) | `7e18b3fc774b7efabf9a20ae3cd6abd0e0715b59` |
| meta-rockchip | <https://github.com/JeffyCN/meta-rockchip> | `697dcfe9898422540ef611c57ce73d178a6b7e56` |

### Rockchip binaries: not included

The boot loader images (`uboot.img`, `loader.bin`) also contain prebuilt Rockchip binaries: DDR
initialisation, USB download helper, first-stage loader (SPL), Trusted Firmware-A (BL31) and
OP-TEE (BL32). They are not part of this repository. They come from the rkbin repository:

- repository: <https://github.com/JeffyCN/mirrors>, branch `rkbin`
- commit `c41b714cacd249e3ef69b2bbe774da5095eefd72`
  (<https://github.com/JeffyCN/mirrors/tree/c41b714cacd249e3ef69b2bbe774da5095eefd72>)

[`u-boot/INCARI/README.md`](u-boot/INCARI/README.md) lists each file used, with its version and
SHA-256.

## Release tarballs

The GitHub release of each tag has one source tarball per component, made with `git archive`
from the tag, and a `SHA256SUMS` file. Each tarball holds this `README.md` and the component's
directory:

- `incari-rk3568-oss-v1.1.2-linux-5.10.198.tar.xz`
- `incari-rk3568-oss-v1.1.2-u-boot-2017.09.tar.xz`
- `incari-rk3568-oss-v1.1.2-rockchip-libmali-1.0-gitaf1d910c.tar.xz`

## How to verify

- Linux kernel: [`linux/INCARI/README.md`](linux/INCARI/README.md) gives the toolchain, the
  exact build commands, the kernel configuration (and how to regenerate it from the in-tree
  defconfig), the SHA-256 of the shipped `Image`, `Image.lz4` and device tree blob, and the
  settings needed for a bit-identical rebuild. It also lists the changes made by Incari GmbH.
- U-Boot: [`u-boot/INCARI/README.md`](u-boot/INCARI/README.md) gives the public base commit, the
  patch series with the origin of each patch, a git tree-id check of the patched tree, the
  configuration, the toolchain, the build script `u-boot/INCARI/build.sh` and the expected
  SHA-256 of `u-boot-nodtb.bin` and `u-boot.dtb`.
- rockchip-libmali: [`rockchip-libmali/INCARI/README.md`](rockchip-libmali/INCARI/README.md) gives the
  public base commit, the patches, the packages that compile the files and their expected
  SHA-256.
