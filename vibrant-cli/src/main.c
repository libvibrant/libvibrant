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
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#include <vibrant/vibrant.h>


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
