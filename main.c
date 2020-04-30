/*
 * vibrantX - Adjust color vibrance of X11 output
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
 * vibrantX is based on color-demo-app written by Leo (Sunpeng) Li <sunpeng.li@amd.com>
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>
#include <libdrm/drm_mode.h>  // need drm_color_ctm

#define RANDR_FORMAT 32u
#define PROP_CTM "CTM"


/**
 * Generate CTM coefficients from double value.
 *
 * Sane values are between 0.0 and 4.0. Everything above 4.0 will massively
 * distort colors.
 *
 * @param saturation Saturation value preferably between 0.0 and 4.0
 * @param coeffs Double array with a length of 9, holding the coefficients
 * will be placed here.
 */
static void saturation_to_coeffs(const double saturation, double *coeffs) {
    double temp[9];

    double coeff = (1.0 - saturation) / 3.0;
    for (int i = 0; i < 9; i++) {
        /* set coefficients. If index is divisible by four (0, 4, 8) add
         * saturation to coeff
         */
        temp[i] = coeff + (i % 4 == 0 ? saturation : 0);
    }

    memcpy(coeffs, temp, sizeof(double) * 9);
}

/**
 * Convert CTM coefficients to human readable "saturation" value.
 *
 * @param coeffs Double array with a length of 9, holding the coefficients
 * @return Saturation value generally between 0.0 and 4.0
 */
static double coeffs_to_saturation(const double *coeffs) {
    /*
     * When calculating the coefficients we add the saturation value to the
     * coefficients with indices 0, 4, 8. This means we can just subtract
     * a coefficient other than 0, 4, 8 from a coefficient at indices 0, 4, 8.
     */

    return coeffs[0] - coeffs[1];
}

/**
 * Translate CTM coefficients to a color CTM format that DRM accepts.
 *
 * DRM requires the CTM to be in signed-magnitude, not 2's complement.
 * It is also in 32.32 fixed-point format.
 * @param coeffs Input coefficients
 * @param ctm DRM CTM struct, used to create the blob. The translated values
 * will be placed here.
 */
static void translate_coeffs_to_ctm(const double *coeffs,
                                    struct drm_color_ctm *ctm) {
    int i;
    for (i = 0; i < 9; i++) {
        if (coeffs[i] < 0) {
            ctm->matrix[i] =
                    (int64_t) (-coeffs[i] * ((uint64_t) 1L << 32u));
            ctm->matrix[i] |= 1ULL << 63u;
        } else
            ctm->matrix[i] =
                    (int64_t) (coeffs[i] * ((uint64_t) 1L << 32u));
    }
}

/**
 * Translate padded color CTM format back to coefficients.
 *
 * DRM requires the CTM to be in signed-magnitude, not 2's complement.
 * It is also in 32.32 fixed-point format.
 * This translates them back to handy-dandy doubles.
 *
 * @param padded_ctm Padded color CTM formatted input
 * @param coeffs Translated coefficients will be placed here.
 */
static void translate_padded_ctm_to_coeffs(const uint64_t *padded_ctm,
                                           double *coeffs) {
    int i;

    for (i = 0; i < 18; i += 2) {
        uint32_t ctm1 = padded_ctm[i];
        uint32_t ctm2 = padded_ctm[i + 1];

        // shove ctm2 and ctm1 into one int64
        uint64_t ctm_n = ((uint64_t) ctm2) << 32u | ctm1;
        // clear sign bit
        ctm_n = (ctm_n & ~(1ULL << 63u));
        // convert fixed-point representation to floating point
        double ctm_d = ctm_n / pow(2.0, 32);
        // recover original sign bit
        if (ctm2 & (1u << 31u))
            ctm_d *= -1;
        coeffs[i / 2] = ctm_d;
    }
}

/**
 * Find the output on the RandR screen resource by name.
 *
 * @param dpy The X Display
 * @param res The RandR screen resource
 * @param name The output name to search for
 * @return The RandR-Output X-ID if found, 0 (None) otherwise
 */
static RROutput find_output_by_name(Display *dpy, XRRScreenResources *res,
                                    const char *name) {
    int i, cmp;
    RROutput ret;
    XRROutputInfo *output_info;

    for (i = 0; i < res->noutput; i++) {
        ret = res->outputs[i];
        output_info = XRRGetOutputInfo(dpy, res, ret);

        cmp = strcmp(name, output_info->name);
        XRRFreeOutputInfo(output_info);

        if (!cmp) {
            return ret;
        }
    }
    return 0;
}

/**
 * Check if output has CTM property.
 *
 * @param dpy The X Display
 * @param output RandR output to get the information from
 * @return 1 if it has a property, 0 if it doesn't or X doesn't support it
 */
static int output_has_ctm(Display *dpy, RROutput output) {
    Atom prop_atom;

    // Find the X Atom associated with the property name
    prop_atom = XInternAtom(dpy, PROP_CTM, 1);
    if (!prop_atom) {
        printf("Property key '%s' not found.\n", PROP_CTM);
        return 0;
    }

    // Make sure the property exists
    if (!XRRQueryOutputProperty(dpy, output, prop_atom)) {
        printf("Property key '%s' not found on output\n", PROP_CTM);
        return 0;
    }
    return 1;
}

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
static int set_output_blob(Display *dpy, RROutput output,
                           const char *prop_name, void *blob_data,
                           size_t blob_bytes) {
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
 * @param blob_data The data of the property blob. The output will be put here.
 * @return X-defined return code
 */
static int get_output_blob(Display *dpy, RROutput output,
                           const char *prop_name, uint64_t *blob_data) {

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

/**
 * Create a DRM color transform matrix using the given coefficients, and set
 * the output's CRTC to use it
 *
 * @param dpy The X Display
 * @param output RandR output to set the property on
 * @param coeffs double array of size 9 containing the coefficients for CTM
 * @return X-defined return code (See set_output_blob())
 */
static int set_ctm(Display *dpy, RROutput output, double *coeffs) {
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

/**
 * Query current CTM values from output's CRTC and convert them to double
 * coefficients.
 *
 * @param dpy The X Display
 * @param output RandR output to set the property on
 * @param coeffs double array of size 9. Will hold the coefficients.
 * @return X-defined return code (See get_output_blob())
 */
static int get_ctm(Display *dpy, RROutput output, double *coeffs) {
    uint64_t padded_ctm[18];
    int ret;

    ret = get_output_blob(dpy, output, PROP_CTM, padded_ctm);

    translate_padded_ctm_to_coeffs(padded_ctm, coeffs);
    return ret;
}

/**
 * Get saturation of output in human readable format.
 * (See saturation_to_coeffs() doc)
 *
 * @param dpy The X Display
 * @param output RandR output to get the saturation from
 * @param x_status X-defined return code (See get_ctm())
 * @return Saturation of output
 */
double get_saturation(Display *dpy, RROutput output, int *x_status) {
    /*
     * These coefficient arrays store a coeff form of the property
     * blob to be set. They will be translated into the format that DDX
     * driver expects when the request is sent to XRandR.
     */
    double ctm_coeffs[9];

    *x_status = get_ctm(dpy, output, ctm_coeffs);

    double saturation = coeffs_to_saturation(ctm_coeffs);

    printf("Current CTM:\n");
    printf("\t%2.4f:%2.4f:%2.4f\n", ctm_coeffs[0], ctm_coeffs[1],
           ctm_coeffs[2]);
    printf("\t%2.4f:%2.4f:%2.4f\n", ctm_coeffs[3], ctm_coeffs[4],
           ctm_coeffs[5]);
    printf("\t%2.4f:%2.4f:%2.4f\n", ctm_coeffs[6], ctm_coeffs[7],
           ctm_coeffs[8]);
    printf("Current Saturation: %f\n", saturation);

    return saturation;
}

/**
 * Get saturation of output in human readable format.
 * (See saturation_to_coeffs() doc)
 *
 * @param dpy The X Display
 * @param output RandR output to set the saturation on
 * @param saturation Saturation of output
 * @param x_status X-defined return code (See get_ctm())
 */
void set_saturation(Display *dpy, RROutput output, double saturation,
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
    printf("\t%2.4f:%2.4f:%2.4f\n", ctm_coeffs[0], ctm_coeffs[1],
           ctm_coeffs[2]);
    printf("\t%2.4f:%2.4f:%2.4f\n", ctm_coeffs[3], ctm_coeffs[4],
           ctm_coeffs[5]);
    printf("\t%2.4f:%2.4f:%2.4f\n", ctm_coeffs[6], ctm_coeffs[7],
           ctm_coeffs[8]);
    printf("New Saturation: %f", saturation);
    *x_status = set_ctm(dpy, output, ctm_coeffs);
}

int main(int argc, char *const argv[]) {
    int x_status;

    char *saturation_opt = NULL;
    char *output_name = NULL;

    // The following values will hold the parsed double from saturation_opt
    char *text;
    double saturation;

    // Values needed for libXRandR
    Display *dpy;
    Window root;
    XRRScreenResources *res;
    RROutput output;


    /*
     * Parse arguments
     */
    if (argc < 2) {
        printf("Usage: %s OUTPUT [SATURATION]\n", argv[0]);
        return 1;
    }
    output_name = argv[1];
    if (argc > 2) {
        saturation_opt = argv[2];

        saturation = strtod(saturation_opt, &text);

        if (strlen(text) > 0 || saturation < 0.0 || saturation > 4.0) {
            printf("SATURATION value must be greater than or equal to 0.0 "
                   "and less than or equal to 4.0.\n");
            return 1;
        }
    }

    /*
     * Open the default X display and window, then obtain the RandR screen
     * resource. Note that the DISPLAY environment variable must exist.
     */
    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        printf("No display specified, check the DISPLAY environment "
               "variable.\n");
        return 1;
    }

    root = DefaultRootWindow(dpy);
    res = XRRGetScreenResourcesCurrent(dpy, root);

    /* RandR needs to know which output we're setting the property on.
     * Since we only have a name to work with, find the RROutput using the
     * name. */
    output = find_output_by_name(dpy, res, output_name);
    if (!output) {
        printf("Cannot find output %s.\n", output_name);
        x_status = BadRequest;
    } else {
        if (output_has_ctm(dpy, output)) {
            get_saturation(dpy, output, &x_status);
            if (saturation_opt != NULL) {
                // set saturation
                set_saturation(dpy, output, saturation, &x_status);
            }
        } else {
            printf("Output does not support saturation.");
        }
    }

    /* Ensure proper cleanup */
    XRRFreeScreenResources(res);
    XCloseDisplay(dpy);

    return x_status;
}
