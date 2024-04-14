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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vibrant/vibrant.h>

/**
 * Find the output on the RandR screen resource by name.
 *
 * @param dpy The X Display
 * @param res The RandR screen resource
 * @param name The output name to search for
 * @return The RandR-Output X-ID if found, 0 (None) otherwise
 */
static vibrant_controller *find_output_by_name(vibrant_controller *controllers,
                                               size_t controllers_size,
                                               const char *name) {
  for (size_t i = 0; i < controllers_size; i++) {
    if (strcmp(name, controllers[i].info->name) == 0) {
      return controllers + i;
    }
  }

  return NULL;
}

int main(int argc, char *const argv[]) {
  // The following values will hold the parsed double from saturation_opt

  printf("libvibrant version %s\n", VIBRANT_VERSION);

  // Parse arguments
  if (argc < 2) {
    printf("Usage: %s OUTPUT [SATURATION]\n", argv[0]);

    return EXIT_FAILURE;
  }

  char *saturation_text;
  double saturation = -1.0;

  char *output_name = argv[1];

  if (argc > 2) {
    char *saturation_opt = argv[2];
    saturation = strtod(saturation_opt, &saturation_text);

    // text will be set to saturation_opt if strtod fails to convert
    if (saturation_text == saturation_opt || saturation < 0.0 ||
        saturation > 4.0) {
      puts("SATURATION value must be between 0.0 and 4.0.");

      return EXIT_FAILURE;
    }
  }

  vibrant_instance *instance;
  vibrant_errors err;
  if ((err = vibrant_instance_new(&instance, NULL)) != vibrant_NoError) {
    switch (err) {
    case vibrant_ConnectToX:
      puts("Failed to connect to default x server.");
      break;
    case vibrant_NoMem:
      puts("Failed to allocate memory for vibrant controller.");
      break;
    default: // satisfy Clang-tidy
      break;
    }

    return EXIT_FAILURE;
  }

  vibrant_controller *controllers;
  size_t controllers_size;
  vibrant_instance_get_controllers(instance, &controllers, &controllers_size);

  /**
   * We need to know which output we're setting the property on.
   * Since we only have a name to work with, find the vibrant_controller
   * using the name.
   */
  vibrant_controller *output =
      find_output_by_name(controllers, controllers_size, output_name);
  if (output == NULL) {
    printf("Cannot find output %s in the list of supported outputs, "
           "it either does not exist or is not supported\n",
           output_name);
    vibrant_instance_free(&instance);
    return EXIT_FAILURE;
  } else {
    if (saturation >= 0) {
      vibrant_controller_set_saturation(output, saturation);
    }
    saturation = vibrant_controller_get_saturation(output);
    printf("Saturation of %s is %f\n", output_name, saturation);
  }

  vibrant_instance_free(&instance);

  return EXIT_SUCCESS;
}
