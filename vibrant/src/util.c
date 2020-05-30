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

#include <math.h>
#include <stdint.h>

#include <libdrm/drm_mode.h>  // need drm_color_ctm

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
static void
vibrant_saturation_to_coeffs(const double saturation, double *coeffs) {
    double coeff = (1.0 - saturation) / 3.0;
    for (int i = 0; i < 9; i++) {
        /* set coefficients. If index is divisible by four (0, 4, 8) add
         * saturation to coeff
         */
        coeffs[i] = coeff + (i % 4 == 0 ? saturation : 0);
    }
}

/**
 * Convert CTM coefficients to human readable "saturation" value.
 *
 * @param coeffs Double array with a length of 9, holding the coefficients
 * @return Saturation value generally between 0.0 and 4.0
 */
static double vibrant_coeffs_to_saturation(const double *coeffs) {
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
static void vibrant_translate_coeffs_to_ctm(const double *coeffs,
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
static void vibrant_translate_padded_ctm_to_coeffs(const uint64_t *padded_ctm,
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

