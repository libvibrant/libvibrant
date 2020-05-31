<img src="assets/vibrant.svg" width="64" alt="Logo" title="vibrant Logo"> vibrant
-------

A simple library to adjust color saturation of X11 outputs.

vibrant, with it's library libvibrant and it's command-line tool vibrant-cli, allows you to adjust the color saturation on X11 outputs, as long as the CTM property is supported.

# Usage
```bash
$ vibrant-cli OUTPUT [SATURATION]
```
Get or set saturation of output.

`OUTPUT` is the name of the X11 output. You can find this by running `xrandr`.
`SATURATION` is a floating point value between (including) 0.0 and (including) 4.0.
- `0.0` or `0` means monochrome
- `1.0` or `1` is normal color saturation (100%)
- if empty the saturation will not be changed

## Examples
### 150% on DisplayPort-0
```bash
$ vibrant-cli DisplayPort-0 1.5
```

### Monochrome on DisplayPort-0
```bash
$ vibrant-cli DisplayPort-0 0
```

### Reset DisplayPort-0
```bash
$ vibrant-cli DisplayPort-0 1
```

### Only query current saturation on DisplayPort-0
```bash
$ vibrant-cli DisplayPort-0
```

# Compatibility
vibrant supports most modern GPUs.
## AMD GPUs
- Your card needs to be supported by the AMD Display Code kernel driver

## NVIDIA GPUs
- Currently only the NVIDIA proprietary driver is supported
- Support is handled by libXNVCtrl, which should be bundled with most installations of the driver

## Confirmed working
- NVIDIA GeForce GTX 660
- Radeon RX 5700 XT
- Radeon RX 5600 XT
- Radeon VII
- Radeon RX Vega 56
- Radeon RX 580
- Radeon RX 470
- Radeon R9 270

# Installation
## Arch Linux
vibrant is available on the Arch Linux User Repository, maintained by me.
- [vibrant](https://aur.archlinux.org/packages/vibrant/)<sup>AUR</sup> - Latest release of the vibrant library and vibrant-cli
- [vibrant-git](https://aur.archlinux.org/packages/vibrant-git/)<sup>AUR</sup> - Latest revision from Git master of the vibrant library and vibrant-cli

## Other Distros
See [Bulding](#Building)

# Building
This project uses CMake.

## Dependencies
- libX11
- libXrandr (possibly bundled with libX11)
- libXNVCtrl (possibly bundled with nvidia-settings)

## Basic building
```bash
$ cd <project directory>
$ mkdir build
$ cd build
$ cmake ..
$ make
```

The binary will be called `vibrant-cli` and will be linked to `libvibrant.so.0`

# License
This project is licensed under the terms of the GNU General Public License 3.0. You can read the full license
text in [LICENSE](LICENSE).

Additionally this project is based on color-demo-app written by Leo (Sunpeng) Li <sunpeng.li@amd.com>, licensed under 
the terms of the MIT license. You can read it's full license text in [NOTICE](NOTICE)
