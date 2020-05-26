/*
 * vibrant - Adjust color vibrance of X11 output
 * Copyright (C) 2020  Sefa Eyeoglu <contact@scrumplex.net> (https://scrumplex.net)
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

#include <glob.h>
#include <stdio.h>
#include <stdint.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>

#ifndef VIBRANT_CTM_H
#define VIBRANT_CTM_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * Set a DRM blob property on the given output. It calls XSync at the end to
 * flush the change request so that it applies.
 *
 * Return values:
 *   - BadAtom if the given name string doesn't exist
 *   - BadName if the property referenced by the name string does not exist
 *   - Success if everything went well
 *
 * @param dpy The X Display
 * @param output RandR output to set the property on
 * @param prop_name String name of the property
 * @param blob_data The data of the property blob
 * @param blob_bytes Size of the data, in bytes
 * @return X-defined return code
 */
int vibrant_set_output_blob(Display *dpy, RROutput output,
                            const char *prop_name, void *blob_data,
                            size_t blob_bytes);

/**
 * Get a DRM blob property on the given output.
 *
 * This method is heavily biased against CTM values. It may not work as is for
 * other properties.
 *
 * Return values:
 *   - BadAtom if the given name string doesn't exist
 *   - BadName if the property referenced by the name string does not exist
 *   - Success if everything went well
 *
 * @param dpy The X Display
 * @param output RandR output to set the property on
 * @param prop_name String name of the property
 * @param blob_data The data of the propeYrty blob. The output will be put here.
 * @return X-defined return code
 */
int vibrant_get_output_blob(Display *dpy, RROutput output,
                            const char *prop_name, uint64_t *blob_data);

/**
 * Create a DRM color transform matrix using the given coefficients, and set
 * the output's CRTC to use it
 *
 * @param dpy The X Display
 * @param output RandR output to set the property on
 * @param coeffs double array of size 9 containing the coefficients for CTM
 * @return X-defined return code (See set_output_blob())
 */
int vibrant_set_ctm(Display *dpy, RROutput output, double *coeffs);

/**
 * Query current CTM values from output's CRTC and convert them to double
 * coefficients.
 *
 * @param dpy The X Display
 * @param output RandR output to set the property on
 * @param coeffs double array of size 9. Will hold the coefficients.
 * @return X-defined return code (See get_output_blob())
 */
static int vibrant_get_ctm(Display *dpy, RROutput output, double *coeffs);

/**
 * Get saturation of output in human readable format.
 * (See saturation_to_coeffs() doc)
 *
 * @param dpy The X Display
 * @param output RandR output to get the saturation from
 * @param x_status X-defined return code (See get_ctm())
 * @return Saturation of output
 */
double vibrant_get_saturation_ctm(Display *dpy, RROutput output, int *x_status);

/**
 * Get saturation of output in human readable format.
 * (See saturation_to_coeffs() doc)
 *
 * @param dpy The X Display
 * @param output RandR output to set the saturation on
 * @param saturation Saturation of output
 * @param x_status X-defined return code (See get_ctm())
 */
void vibrant_set_saturation_ctm(Display *dpy, RROutput output,
                                double saturation, int *x_status);

/**
 * Check if output has the CTM property.
 *
 * @param dpy The X Display
 * @param output RandR output to get the information from
 * @return 1 if it has a property, 0 if it doesn't or X doesn't support it
 */
int vibrant_output_has_ctm(Display *dpy, RROutput output);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // VIBRANT_CTM_H
