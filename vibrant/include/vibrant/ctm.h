/*
 * vibrant - Adjust color vibrancy of X11 output
 * Copyright (C) 2020  Sefa Eyeoglu <contact@scrumplex.net> (https://scrumplex.net)
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

/*
 * vibrant is based on color-demo-app written by Leo (Sunpeng) Li <sunpeng.li@amd.com>
 *
 * Original license text of color-demo-app:
 *
 * Copyright 2018 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: AMD
 *
 */
// user code should NOT include this header directly, just use the interfaces
// provided through vibrant.h

#include <stdint.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#ifndef LIBVIBRANT_CTM_H
#define LIBVIBRANT_CTM_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/*
 * From drm/drm_mode.h
 */
struct drm_color_ctm {
    /*
     * Conversion matrix in S31.32 sign-magnitude
     * (not two's complement!) format.
     */
    uint64_t matrix[9];
};

/**
 * Get saturation of output in human readable format.
 * (See saturation_to_coeffs() doc)
 *
 * @param dpy The X Display
 * @param output RandR output to get the saturation from
 * @param x_status X-defined return code (See get_ctm())
 * @return Saturation of output
 */
double ctm_get_saturation(Display *dpy, RROutput output, int32_t *x_status);

/**
 * Get saturation of output in human readable format.
 * (See saturation_to_coeffs() doc)
 *
 * @param dpy The X Display
 * @param output RandR output to set the saturation on
 * @param saturation Saturation of output
 * @param x_status X-defined return code (See get_ctm())
 */
void ctm_set_saturation(Display *dpy, RROutput output,
                        double saturation, int32_t *x_status);

/**
 * Check if output has the CTM property.
 *
 * @param dpy The X Display
 * @param output RandR output to get the information from
 * @return 1 if it has a property, 0 if it doesn't or X doesn't support it
 */
int32_t ctm_output_has_ctm(Display *dpy, RROutput output);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // LIBVIBRANT_CTM_H
