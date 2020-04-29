vibrantX
--------
Adjust color vibrance of X11 output

vibrantX is a simple tool to adjust the color vibrance (or color saturation) of X11 outputs.

# Usage
```bash
$ vibrantX OUTPUT [SATURATION]
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
$ vibrantX DisplayPort-0 1.5
```

### Monochrome on DisplayPort-0
```bash
$ vibrantX DisplayPort-0 0
```

### Reset DisplayPort-0
```bash
$ vibrantX DisplayPort-0 1
```

### Only query current saturation on DisplayPort-0
```bash
$ vibrantX DisplayPort-0
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

# General TODOs
- Are there any possible buffer overflows?

# License
This project is licensed under the terms of the GNU General Public License 3.0. You can read the full license
text in [LICENSE](LICENSE).

Additionally this project is based on color-demo-app written by Leo (Sunpeng) Li <sunpeng.li@amd.com>, licensed under 
the terms of the MIT license. You can read it's full license text in [NOTICE](NOTICE)
