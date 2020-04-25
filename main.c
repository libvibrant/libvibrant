/*
 * Copyright 2018 Advanced Micro Devices, Inc.
 * Copyright 2020 Sefa Eyeoglu <contact@scrumplex.net> (https://scrumplex.net)
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
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>

#define RANDR_FORMAT 32u
#define PROP_CTM "CTM"

/**
 * The below data structures are identical to the ones used by DRM. They are
 * here to help us structure the data being passed to the kernel.
 */
struct drm_color_ctm {
    /* Transformation matrix in S31.32 format. */
    uint64_t matrix[9];
};



/*******************************************************************************
 * Helper functions
 */

/**
 * Translate coefficients to a color CTM format that DRM accepts.
 *
 * DRM requres the CTM to be in signed-magnitude, not 2's complement.
 * It is also in 31.32 fixed-point format.
 *
 * @coeffs: Input coefficients
 * @ctm: DRM CTM struct, used to create the blob. The translated values will be
 *       placed here.
 */
static void coeffs_to_ctm(const double *coeffs,
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
 * Find the output on the RandR screen resource by name.
 *
 * dpy: The X display
 * res: The RandR screen resource
 * name: The output name to search for.
 *
 * Return: The RROutput X-id if found, 0 (None) otherwise.
 */
static RROutput find_output_by_name(Display *dpy, XRRScreenResources *res,
                                    const char *name) {
    int i;
    RROutput ret;
    XRROutputInfo *output_info;

    for (i = 0; i < res->noutput; i++) {
        ret = res->outputs[i];
        output_info = XRRGetOutputInfo(dpy, res, ret);

        if (!strcmp(name, output_info->name)) {
            XRRFreeOutputInfo(output_info);
            return ret;
        }

        XRRFreeOutputInfo(output_info);
    }
    return 0;
}


/**
 * Set a DRM blob property on the given output. It calls XSync at the end to
 * flush the change request so that it applies.
 *
 * @dpy: The X Display
 * @output: RandR output to set the property on
 * @prop_name: String name of the property.
 * @blob_data: The data of the property blob.
 * @blob_bytes: Size of the data, in bytes.
 * @format: Format of each element within blob_data.
 *
 * Return: X-defined return codes:
 *     - BadAtom if the given name string doesn't exist.
 *     - BadName if the property referenced by the name string does not exist on
 *       the given connector
 *     - Success otherwise.
 */
static int set_output_blob(Display *dpy, RROutput output,
                           const char *prop_name, void *blob_data,
                           size_t blob_bytes) {
    Atom prop_atom;
    XRRPropertyInfo *prop_info;

    /* Find the X Atom associated with the property name */
    prop_atom = XInternAtom(dpy, prop_name, 1);
    if (!prop_atom) {
        printf("Property key '%s' not found.\n", prop_name);
        return BadAtom;
    }

    /* Make sure the property exists */
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
    XRRChangeOutputProperty(dpy, output, prop_atom,
                            XA_INTEGER, RANDR_FORMAT, PropModeReplace,
                            blob_data, blob_bytes / (RANDR_FORMAT >> 3u));
    /* Call XSync to apply it. */
    XSync(dpy, 0);

    return Success;
}

/**
 * Create a DRM color transform matrix using the given coefficients, and set
 * the output's CRTC to use it.
 */
static int set_ctm(Display *dpy, RROutput output, double *coeffs) {
    size_t blob_size = sizeof(struct drm_color_ctm);
    struct drm_color_ctm ctm;
    long padded_ctm[18];

    int i, ret;

    coeffs_to_ctm(coeffs, &ctm);

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
        /* Think of this as a padded 'memcpy()'. */
        padded_ctm[i] = ((uint32_t *) ctm.matrix)[i];

    ret = set_output_blob(dpy, output, PROP_CTM, &padded_ctm, blob_size);

    if (ret)
        printf("Failed to set CTM. %d\n", ret);
    return ret;
}

/*******************************************************************************
 * main function, and functions to assist in parsing input.
 */

void saturation_to_ctm(double saturation, double *coeffs) {
    double temp[9];

    double coeff = (1.0 - saturation) / 3.0;
    for (int i = 0; i < 9; i++) {
        // set coefficients. If index is divisible by four (0, 4, 8) add saturation to coeff
        temp[i] = coeff + (i % 4 == 0 ? saturation : 0);
    }

    memcpy(coeffs, temp, sizeof(double) * 9);
}

int main(int argc, char *const argv[]) {
    /* These coefficient arrays store an coeff form of the property
     * blob to be set. They will be translated into the format that DDX
     * driver expects when the request is sent to XRandR.
     */
    double ctm_coeffs[9];

    int ret;

    /* Things needed by xrandr to change output properties */
    Display *dpy;
    Window root;
    XRRScreenResources *res;
    RROutput output;


    /*
     * Parse arguments
     */
    if (argc < 3) {
        printf("Usage: %s SATURATION DISPLAY", argv[0]);
        return 1;
    }

    double sat_opt = strtod(argv[1], NULL);
    char *output_name = argv[2];

    saturation_to_ctm(sat_opt, ctm_coeffs);  // convert saturation to ctm coefficients

    printf("Calculated CTM:\n");
    printf("    %2.4f:%2.4f:%2.4f\n", ctm_coeffs[0], ctm_coeffs[1], ctm_coeffs[2]);
    printf("    %2.4f:%2.4f:%2.4f\n", ctm_coeffs[3], ctm_coeffs[4], ctm_coeffs[5]);
    printf("    %2.4f:%2.4f:%2.4f\n", ctm_coeffs[6], ctm_coeffs[7], ctm_coeffs[8]);

    /* Open the default X display and window, then obtain the RandR screen
     * resource. Note that the DISPLAY environment variable must exist. */
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
        ret = 1;
    } else {
        /* Set the properties as parsed. The set_* functions will also
         * translate the coefficients. */
        ret = set_ctm(dpy, output, ctm_coeffs);
    }

    /* Ensure proper cleanup */
    XRRFreeScreenResources(res);
    XCloseDisplay(dpy);

    return ret;
}
