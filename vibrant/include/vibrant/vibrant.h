/*
 * vibrant - Adjust color vibrance of X11 output
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

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#ifndef LIBVIBRANT_VIBRANT_H
#define LIBVIBRANT_VIBRANT_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//private structs, users don't need and shouldn't be accessing their data
typedef struct vibrant_instance vibrant_instance;
struct vibrant_controller_internal;

typedef enum vibrant_errors {
    vibrant_NoError,
    vibrant_ConnectToX,
    vibrant_NoMem
} vibrant_errors;

typedef struct vibrant_controller {
    RROutput output;
    XRROutputInfo *info;
    //copy of display of the owning vibrant_instance
    Display *display;

    struct vibrant_controller_internal *priv;
} vibrant_controller;

/**
 * initializes a vibrant_instance struct using the X server specified by
 * display_name.
 * @param instance
 * @param display_name
 * @return vibrant_NoError if no issues occurred, vibrant_connectToX if
 * connecting to display_name failed, or vibrant_NoMem if memory allocation
 * failed
 */
vibrant_errors vibrant_instance_new(vibrant_instance **instance,
        const char *display_name);

/**
 * Deinits instance by closing its X connection and freeing its allocated
 * memory.
 * @param instance
 */
void vibrant_instance_free(vibrant_instance **instance);

/**
 * Sets controllers to be a pointer to an array of length controllers
 * @param instance
 * @param controllers
 * @param length
 */
void vibrant_instance_get_controllers(vibrant_instance *instance,
                                      vibrant_controller **controllers,
                                      size_t *length);

/**
 * Returns a double in the range of [0.0, 4.0] representing the current
 * saturation. 0.0 being no saturation, 1.0 being the default,
 * and 4.0 being max saturation
 * @param controller
 */
double vibrant_controller_get_saturation(vibrant_controller *controller);

/**
 * Sets the the saturation of the display controlled by controller. The accepted
 * value ranges are [0.0, 4.0] with 0.0 being no saturation, 1.0 being the
 * default, and 4.0 being max saturation
 * @param controller
 * @param saturation
 */
void vibrant_controller_set_saturation(vibrant_controller *controller,
                                       double saturation);


#ifdef __cplusplus
}
#endif // __cplusplus
#endif // LIBVIBRANT_VIBRANT_H
