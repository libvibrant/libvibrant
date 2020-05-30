/*
 * vibrant - Adjust color vibrancy of X11 output
 * Copyright (C) 2020  zee
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// user code should NOT include this header directly, just use the interfaces
// provided through vibrant.h

#ifndef LIBVIBRANT_NVIDIA_H
#define LIBVIBRANT_NVIDIA_H

#include <stdint.h>
#include <X11/Xlib.h>

double nvidia_get_saturation(Display *dpy, int32_t id);

void nvidia_set_saturation(Display *dpy, int32_t id, double saturation);

#endif //LIBVIBRANT_NVIDIA_H
