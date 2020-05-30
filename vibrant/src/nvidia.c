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

#include "vibrant/nvidia.h"

#include <NVCtrl/NVCtrlLib.h>
#include <float.h>

double nvidia_get_saturation(Display *dpy, int id) {
    int nv_saturation;
    double saturation;
    XNVCTRLQueryTargetAttribute(dpy, NV_CTRL_TARGET_TYPE_DISPLAY, id, 0,
                                NV_CTRL_DIGITAL_VIBRANCE, &nv_saturation);

    if (nv_saturation < 0) {
        saturation = (double) (nv_saturation + 1024) / 1024;
    } else {
        saturation = (double) (nv_saturation * 3 + 1023) / 1023;
    }

    return saturation;
}

void nvidia_set_saturation(Display *dpy, int id, double saturation) {
    int nv_saturation;
    //is saturation roughly in [0.0, 1.0]
    if (saturation >= 0.0 && saturation <= 1.0 + DBL_EPSILON) {
        nv_saturation = saturation * 1024 - 1024;
    } else {
        nv_saturation = (saturation * 1023 - 1023) / 3;
    }

    XNVCTRLSetTargetAttribute(dpy, NV_CTRL_TARGET_TYPE_DISPLAY, id, 0,
                              NV_CTRL_DIGITAL_VIBRANCE, nv_saturation);
}
