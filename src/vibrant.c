/*
 * vibrant - Adjust color vibrancy of X11 output
 * Copyright (C) 2020  Sefa Eyeoglu <contact@scrumplex.net>
 * (https://scrumplex.net) Copyright (C) 2020  zee
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
 * vibrant is based on color-demo-app written by Leo (Sunpeng) Li
 * <sunpeng.li@amd.com>
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

#include "vibrant/vibrant.h"
#include "vibrant/ctm.h"
#include "vibrant/nvidia.h"

#include <NVCtrl/NVCtrlLib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef double (*vibrant_get_saturation_fn)(vibrant_controller *);

typedef void (*vibrant_set_saturation_fn)(vibrant_controller *, double);

double ctmctrl_get_saturation(vibrant_controller *controller);

void ctmctrl_set_saturation(vibrant_controller *controller, double saturation);

double nvctrl_get_saturation(vibrant_controller *controller);

void nvctrl_set_saturation(vibrant_controller *controller, double saturation);

typedef enum vibrant_controller_backend {
  CTM,
  XNVCtrl,
  Unknown
} vibrant_controller_backend;

typedef struct vibrant_controller_internal {
  vibrant_controller_backend backend;

  // only applied if this display is an nvidia display,
  // otherwise this is set to -1
  int nvId;

  vibrant_get_saturation_fn get_saturation;
  vibrant_set_saturation_fn set_saturation;
} vibrant_controller_internal;

struct vibrant_instance {
  Display *dpy;

  vibrant_controller *controllers;
  int controllers_size;
};

vibrant_errors vibrant_instance_new(vibrant_instance **instance,
                                    const char *display_name) {
  *instance = malloc(sizeof(vibrant_instance));
  if (*instance == NULL) {
    return vibrant_NoMem;
  }

  Display *dpy = XOpenDisplay(display_name);
  if (dpy == NULL) {
    free(*instance);

    return vibrant_ConnectToX;
  }

  bool dpy_has_nvidia = XNVCTRLQueryExtension(dpy, NULL, NULL);

  Window root = DefaultRootWindow(dpy);
  XRRScreenResources *resources = XRRGetScreenResources(dpy, root);

  vibrant_controller *controllers =
      malloc(sizeof(vibrant_controller) * resources->noutput);
  if (controllers == NULL) {
    XRRFreeScreenResources(resources);
    XCloseDisplay(dpy);
    free(*instance);

    return vibrant_NoMem;
  }
  int controllers_size = resources->noutput;

  // filter out disconnected outputs
  int n_connected = 0;
  for (int i = 0; i < controllers_size; i++) {
    XRROutputInfo *info =
        XRRGetOutputInfo(dpy, resources, resources->outputs[i]);

    if (info->connection == RR_Connected) {
      vibrant_controller_internal *priv =
          malloc(sizeof(vibrant_controller_internal));
      if (priv == NULL) {
        // free already added controllers
        for (int j = 0; j < n_connected; j++) {
          XRRFreeOutputInfo(controllers[j].info);
          free(controllers[j].priv);
        }

        // free everything else
        XRRFreeScreenResources(resources);
        XCloseDisplay(dpy);
        free(controllers);
        free(*instance);

        return vibrant_NoMem;
      }

      *priv = (vibrant_controller_internal){Unknown, -1};
      controllers[n_connected] =
          (vibrant_controller){resources->outputs[i], info, dpy, priv};
      n_connected++;
    } else {
      XRRFreeOutputInfo(info);
    }
  }

  // resize to only include connected displays
  vibrant_controller *tmp =
      realloc(controllers, sizeof(vibrant_controller) * n_connected);

  /**
   * realloc may return NULL if size is 0, depending on the C implementation.
   * realloc with size 0 is equivalent to free, except for the return value.
   * Let's force this, so that we don't get any broken pointers.
   */
  if (n_connected == 0) {
    controllers = NULL;
  }

  if (n_connected > 0 && tmp == NULL) {
    for (int i = 0; i < n_connected; i++) {
      XRRFreeOutputInfo(controllers[i].info);
      free(controllers[i].priv);
    }

    XRRFreeScreenResources(resources);
    XCloseDisplay(dpy);
    free(controllers);
    free(*instance);

    return vibrant_NoMem;
  }

  controllers = tmp;
  controllers_size = n_connected;

  /**
   * Check all available screens and outputs if they are managed by NVIDIA.
   * If they are set their backend and nvIds
   */
  if (dpy_has_nvidia) {
    for (int i = 0; i < ScreenCount(dpy); i++) {
      if (XNVCTRLIsNvScreen(dpy, i)) {
        int *nvDpyIds;
        int nvDpyIdsLen;

        /**
         * nvDpyIdsLen will contain how many bytes are inside of
         * nvDpyIds and the first element in nvDpyIds will tells us how
         * many *elements* are in the array. so on a two display system
         * nvDpyIdsLen will be 12 bytes and nvDpyIds will contain
         * 3 elements in the format of [2, first_dpy_id, second_dpy_id]
         */
        XNVCTRLQueryBinaryData(dpy, i, 0,
                               NV_CTRL_BINARY_DATA_DISPLAYS_ENABLED_ON_XSCREEN,
                               (unsigned char **)&nvDpyIds, &nvDpyIdsLen);
        for (int j = 1; j <= nvDpyIds[0]; j++) {
          int output;
          XNVCTRLQueryTargetAttribute(dpy, NV_CTRL_TARGET_TYPE_DISPLAY,
                                      nvDpyIds[j], 0,
                                      NV_CTRL_DISPLAY_RANDR_OUTPUT_ID, &output);
          for (size_t k = 0; k < controllers_size; k++) {
            if (controllers[k].output == output) {
              controllers[k].priv->backend = XNVCtrl;
              controllers[k].priv->nvId = nvDpyIds[j];
              controllers[k].priv->get_saturation = nvctrl_get_saturation;
              controllers[k].priv->set_saturation = nvctrl_set_saturation;
            }
          }
        }
      }
    }
  }

  /**
   * Check remaining outputs if they support the CTM setting and set their
   * respective backend
   */
  for (size_t i = 0; i < controllers_size; i++) {
    if (controllers[i].priv->backend == Unknown) {
      if (ctm_output_has_ctm(dpy, controllers[i].output)) {
        controllers[i].priv->backend = CTM;
        controllers[i].priv->get_saturation = ctmctrl_get_saturation;
        controllers[i].priv->set_saturation = ctmctrl_set_saturation;
      }
    }
  }

  /**
   * Remove all remaining outputs, as they are not supported.
   */
  for (int i = 0; i < controllers_size; i++) {
    if (controllers[i].priv->backend == Unknown) {
      XRRFreeOutputInfo(controllers[i].info);
      free(controllers[i].priv);

      // move all controllers after i one "to the left"
      memmove(controllers + i, controllers + i + 1,
              sizeof(vibrant_controller) * (controllers_size - i - 1));
      controllers_size--;
      i--; // stay at current index, as we just popped this index
    }
  }

  tmp = realloc(controllers, sizeof(vibrant_controller) * controllers_size);

  /**
   * realloc may return NULL if size is 0, depending on the C implementation.
   * realloc with size 0 is equivalent to free, except for the return value.
   * Let's force this, so that we don't get any broken pointers.
   */
  if (controllers_size == 0) {
    controllers = NULL;
  }

  if (controllers_size > 0 && tmp == NULL) {
    for (int i = 0; i < controllers_size; i++) {
      XRRFreeOutputInfo(controllers[i].info);
      free(controllers[i].priv);
    }

    XRRFreeScreenResources(resources);
    XCloseDisplay(dpy);
    free(controllers);
    free(*instance);

    return vibrant_NoMem;
  }

  **instance = (vibrant_instance){dpy, controllers, controllers_size};
  XRRFreeScreenResources(resources);

  return vibrant_NoError;
}

void vibrant_instance_free(vibrant_instance **instance) {
  for (int i = 0; i < (*instance)->controllers_size; i++) {
    XRRFreeOutputInfo((*instance)->controllers[i].info);
    free((*instance)->controllers[i].priv);
  }

  free((*instance)->controllers);
  XCloseDisplay((*instance)->dpy);

  free(*instance);
  instance = NULL;
}

void vibrant_instance_get_controllers(vibrant_instance *instance,
                                      vibrant_controller **controllers,
                                      size_t *length) {
  *controllers = instance->controllers;
  *length = instance->controllers_size;
}

double vibrant_controller_get_saturation(vibrant_controller *controller) {
  return controller->priv->get_saturation(controller);
}

void vibrant_controller_set_saturation(vibrant_controller *controller,
                                       double saturation) {
  controller->priv->set_saturation(controller, saturation);
}

double ctmctrl_get_saturation(vibrant_controller *controller) {
  return ctm_get_saturation(controller->display, controller->output, NULL);
}

void ctmctrl_set_saturation(vibrant_controller *controller, double saturation) {
  ctm_set_saturation(controller->display, controller->output, saturation, NULL);
}

double nvctrl_get_saturation(vibrant_controller *controller) {
  return nvidia_get_saturation(controller->display, controller->priv->nvId);
}

void nvctrl_set_saturation(vibrant_controller *controller, double saturation) {
  nvidia_set_saturation(controller->display, controller->priv->nvId,
                        saturation);
}
