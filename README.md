vibrantX
--------
Adjust color vibrance of X11 output

vibrantX is a simple tool to adjust the color vibrance (or color saturation) of X11 outputs.

# Usage
```bash
$ vibrantX SATURATION OUTPUT
```
`SATURATION` is a floating point value between (including) 0.0 and (including) 4.0.
- `0.0` or `0` means monochrome
- `1.0` or `1` is normal color saturation (100%)

`OUTPUT` is the name of the X11 output. You can find this by running `xrandr`.

## Examples
### 200% on DisplayPort-0
```bash
$ vibrantX 2 DisplayPort-0
```

### Monochrome on DisplayPort-0
```bash
$ vibrantX 0 DisplayPort-0
```

### Reset DisplayPort-0
```bash
$ vibrantX 1 DisplayPort-0
```

# Building
This project uses CMake.

## Basic building
```bash
$ cd <project directory>
$ mkdir build
$ cd build
$ cmake ..
$ make
```

The binary will be called `vibrantX`

# License
This project is licensed under the terms of the GNU General Public License 3.0. You can read the full license
text in [LICENSE](LICENSE).

Additionally this project is based on color-demo-app written by Leo (Sunpeng) Li <sunpeng.li@amd.com>, licensed under 
the terms of the MIT license. You can read it's full license text in [NOTICE](NOTICE)
