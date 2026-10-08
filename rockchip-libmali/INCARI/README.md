# rockchip-libmali: `include/EGL/eglext.h` as compiled for Incari RK3568 OS v1.1.2

This directory holds one file of Rockchip's Mali user-space driver package (libmali, Yocto recipe
`rockchip-libmali` 1.0): the Khronos EGL extension header `include/EGL/eglext.h`, exactly as it was
compiled for Incari RK3568 OS v1.1.2, and the patch Incari GmbH applied to it. The header is
compiled into open-source packages of the OS (below), so its changed version is published here. The
Mali driver library itself is a prebuilt binary under its own licence; it is not part of this
repository.

## Where it is compiled in

The package `rockchip-libmali-dev` installs the header as `/usr/include/EGL/eglext.h`. These
packages of the OS are compiled with it (the build's SPDX documents and the debug information of
their binaries):

| Package | Yocto recipe | Version | Shipped files compiled with it |
|---------|--------------|---------|--------------------------------|
| `libgstgl-1.0-0` | gstreamer1.0-plugins-base | 1.20.7 | `/usr/lib/libgstgl-1.0.so.0.2007.0` |
| `libweston-10` | weston | 10.0.2 | `/usr/lib/libweston-10.so.0.0.2`, `/usr/lib/libweston-10/gl-renderer.so` |

## Source

Base: Rockchip's libmali tree, mirrored at

- repository: <https://github.com/JeffyCN/mirrors>, branch `libmali`
- commit `af1d910cd29d52e354f6679e46d1bf2bbc7ca476`
  ("meson: Fix rpath error", 2025-02-26)

In the base, the file is the Khronos EGL registry header (registry commit `cb927ca98d` of
2019-08-08) that Rockchip ships with libmali.

Patches applied on top, in this order (`INCARI/patches/series`, all `-p1`):

| # | Patch | Origin |
|---|-------|--------|
| 1 | [`0001-wlroot-bug-fix.patch`](patches/0001-wlroot-bug-fix.patch) | Change made by Incari GmbH for Incari RK3568 OS (dated 2024-08-22): adds the declarations of `EGL_WL_bind_wayland_display` and `EGL_WL_create_wayland_buffer_from_image` and re-formats the header. |

The patch adds the declarations of the EGL extensions `EGL_WL_bind_wayland_display` and
`EGL_WL_create_wayland_buffer_from_image` at the end of the header. They are the declarations the
Khronos EGL registry generates for these extensions: token for token, apart from one line
continuation, those of the `eglext.h` of registry commit `6fb1daea15` (2022-05-25), which Rockchip's
libmali took over later, in commit `3987a274f73d` ("{EGL, GLES}: Bump headers", 2025-03-12). The
rest of the patch re-formats the header; it changes no other declaration.

## Licence

The file keeps the licence in its header: the Khronos Group's MIT-style licence ("Copyright (c)
2013-2017 The Khronos Group Inc."; ScanCode licence key `khronos`). Incari GmbH distributes its
change to the file under the same licence. The later Khronos header whose declarations the two added
blocks match (libmali commit `3987a274f73d`, Khronos registry commit `6fb1daea15`) carries a
different notice: "Copyright 2013-2020 The Khronos Group Inc." and "SPDX-License-Identifier:
Apache-2.0".

## How to verify

The published file, as compiled (SHA-256; the same as the file the build's `rockchip-libmali-dev`
package installs):

| File | SHA-256 |
|------|---------|
| `rockchip-libmali/include/EGL/eglext.h` | `028bafdbc0186421435aa02a3961e8c36ea6c3edb2110dbdd51efa279646e650` |

It is the base file with the patches applied. From the public base commit (set `P` to the absolute
path of `rockchip-libmali/INCARI/patches` in a clone of this repository or in the release tarball):

```sh
mkdir rockchip-libmali-check && cd rockchip-libmali-check
mkdir -p include/EGL && curl -fsSL -o include/EGL/eglext.h \
    https://raw.githubusercontent.com/JeffyCN/mirrors/af1d910cd29d52e354f6679e46d1bf2bbc7ca476/include/EGL/eglext.h
while read -r patch opt; do
    patch $opt --no-backup-if-mismatch -s -i "$P/$patch"
done < "$P/series"
sha256sum include/EGL/eglext.h
```
