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

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>

#include "util.c"
#include "utilx.c"

#include "vibrant/ctm.h"

#define RANDR_FORMAT 32u
#define PROP_CTM "CTM"

int set_output_blob(Display *dpy, RROutput output, const char *prop_name,
                    void *blob_data, size_t blob_bytes) {
    Atom prop_atom;
    XRRPropertyInfo *prop_info;

    // Find the X Atom associated with the property name
    prop_atom = XInternAtom(dpy, prop_name, 1);
    if (!prop_atom) {
        printf("Property key '%s' not found.\n", prop_name);
        return BadAtom;
    }

    // Make sure the property exists
    prop_info = XRRQueryOutputProperty(dpy, output, prop_atom);
    if (!prop_info) {
        printf("Property key '%s' not found on output\n", prop_name);
        return BadName;  /* Property not found */
    }

    /* Change the property
     *
     * Due to some restrictions in RandR, array properties of 32-bit format
     * must be of type 'long'. See set_ctm() for details.
     *
     * To get the number of elements within blob_data, we take its size in
     * bytes, divided by the size of one of it's elements in bytes:
     *
     * blob_length = blob_bytes / (element_bytes)
     *             = blob_bytes / (format / 8)
     *             = blob_bytes / (format >> 3)
     */
    XRRChangeOutputProperty(dpy, output, prop_atom, XA_INTEGER, RANDR_FORMAT,
                            PropModeReplace, blob_data,
                            blob_bytes / (RANDR_FORMAT >> 3u));
    // Call XSync to apply it.
    XSync(dpy, 0);

    return Success;
}

int get_output_blob(Display *dpy, RROutput output, const char *prop_name,
                    uint64_t *blob_data) {

    int ret, actual_format;
    unsigned long n_items, bytes_after;
    unsigned char *buffer;
    Atom prop_atom, actual_type;
    XRRPropertyInfo *prop_info;

    // Find the X Atom associated with the property name
    prop_atom = XInternAtom(dpy, prop_name, 1);
    if (!prop_atom) {
        printf("Property key '%s' not found.\n", prop_name);
        return BadAtom;
    }

    // Make sure the property exists
    prop_info = XRRQueryOutputProperty(dpy, output, prop_atom);
    if (!prop_info) {
        printf("Property key '%s' not found on output\n", prop_name);
        return BadName;  /* Property not found */
    }

    // Get the property
    ret = XRRGetOutputProperty(dpy, output, prop_atom, 0, sizeof(uint32_t) * 18,
                               0, 0, XA_INTEGER,
                               &actual_type, &actual_format,
                               &n_items, &bytes_after, &buffer);
    if (actual_type == XA_INTEGER && actual_format == RANDR_FORMAT &&
        n_items == 18) {
        for (int i = 0; i < 18; i++) {
            /*
             * Due to some restrictions in RandR, array properties of 32-bit format
             * must be of type 'long'. See set_ctm() for details.
             */
            blob_data[i] = ((uint64_t *) buffer)[i];
        }
        return Success;
    }

    return ret;
}

int set_ctm(Display *dpy, RROutput output, double *coeffs) {
    size_t blob_size = sizeof(struct drm_color_ctm);
    struct drm_color_ctm ctm;
    long padded_ctm[18];

    int i, ret;

    translate_coeffs_to_ctm(coeffs, &ctm);

    /* Workaround:
     *
     * RandR currently uses long types for 32-bit integer format. However,
     * 64-bit systems will use 64-bits for long, causing data corruption
     * once RandR parses the data. Therefore, pad the blob_data to be long-
     * sized. This will work regardless of how long is defined (as long as
     * it's at least 32-bits).
     *
     * Note that we have a 32-bit format restriction; we have to interpret
     * each S31.32 fixed point number within the CTM in two parts: The
     * whole part (S31), and the fractional part (.32). They're then stored
     * (as separate parts) into a long-typed array. Of course, This problem
     * wouldn't exist if xserver accepted 64-bit formats.
     *
     * A gotcha here is the endianness of the S31.32 values. The whole part
     * will either come before or after the fractional part. (before in
     * big-endian format, and after in small-endian format). We could avoid
     * dealing with this by doing a straight memory copy, but we have to
     * ensure that each 32-bit element is padded to long-size in the
     * process.
     */
    for (i = 0; i < 18; i++)
        // Think of this as a padded 'memcpy()'.
        padded_ctm[i] = ((uint32_t *) ctm.matrix)[i];

    ret = set_output_blob(dpy, output, PROP_CTM, &padded_ctm, blob_size);

    if (ret)
        printf("Failed to set CTM. %d\n", ret);
    return ret;
}

int get_ctm(Display *dpy, RROutput output, double *coeffs) {
    uint64_t padded_ctm[18];
    int ret;

    ret = get_output_blob(dpy, output, PROP_CTM, padded_ctm);

    translate_padded_ctm_to_coeffs(padded_ctm, coeffs);
    return ret;
}

double get_saturation_ctm(Display *dpy, RROutput output, int *x_status) {
    /*
     * These coefficient arrays store a coeff form of the property
     * blob to be set. They will be translated into the format that DDX
     * driver expects when the request is sent to XRandR.
     */
    double ctm_coeffs[9];

    *x_status = get_ctm(dpy, output, ctm_coeffs);

    double saturation = coeffs_to_saturation(ctm_coeffs);

    printf("Current CTM:\n");
    print_ctm_coeffs(ctm_coeffs, saturation);

    return saturation;
}

void set_saturation_ctm(Display *dpy, RROutput output, double saturation,
                        int *x_status) {
    /*
     * These coefficient arrays store a coeff form of the property
     * blob to be set. They will be translated into the format that DDX
     * driver expects when the request is sent to XRandR.
     */
    double ctm_coeffs[9];

    // convert saturation to ctm coefficients
    saturation_to_coeffs(saturation, ctm_coeffs);

    printf("New CTM:\n");
    print_ctm_coeffs(ctm_coeffs, saturation);

    *x_status = set_ctm(dpy, output, ctm_coeffs);
}

int output_has_ctm(Display *dpy, RROutput output) {
    return output_has_property(dpy, output, PROP_CTM);
}
