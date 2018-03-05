/*
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

#include <xf86drm.h>
#include <stdio.h>
#include <math.h>


#define LUT_SIZE 16


/* Data structures for gamma/degamma ramps & ctm matrix. */
struct _drm_color_ctm {
	/* Transformation matrix in S31.32 format. */
	__s64 matrix[9];
};

struct _drm_color_lut {
	/*
	 * Data is U0.16 fixed point format.
	 */
	__u16 red;
	__u16 green;
	__u16 blue;
	__u16 reserved;
};

struct color3d {
	double r;
	double g;
	double b;
};


/**
 * Helper functions
 */

static void coeffs_to_lut(struct color3d *coeffs,
			  struct _drm_color_lut *lut,
			  int lut_size)
{
	int i;
	__u16 max_value = 0xFFFF;

	for (i = 0; i < lut_size; i++) {
		lut[i].red = coeffs[i].r * max_value;
		lut[i].green = coeffs[i].g * max_value;
		lut[i].blue = coeffs[i].b * max_value;
	}
}

static void load_table_max(struct color3d *coeffs, int lut_size)
{
	int i;
	coeffs[0].r = coeffs[0].g = coeffs[0].b = 0.0;
	for (i = 1; i < lut_size; i++)
		coeffs[i].r = coeffs[i].g = coeffs[i].b = 1.0;
}

static void load_table_zero(struct color3d *coeffs, int lut_size)
{
	int i;
	for (i = 0; i < lut_size; i++)
		coeffs[i].r = coeffs[i].g = coeffs[i].b = 0.0;
}

static void load_table(struct color3d *coeffs, int lut_size, double exp)
{
	int i;
	for (i = 0; i < lut_size; i++) {
		coeffs[i].r = coeffs[i].g = coeffs[i].b =
			pow((double) i * 1.0 / (double) (lut_size - 1), exp);
	}
}

static void print_coeffs(const struct color3d *coeffs, int lut_size)
{
	int i;
	for (i = 0; i < lut_size; i++) {
		printf("[%d] R:%.2f G:%.2f B:%.2f\n", 
		       i, coeffs[i].r, coeffs[i].g, coeffs[i].b);
	}
}

static void print_lut(const struct _drm_color_lut *lut, int lut_size)
{
	int i;
	for (i = 0; i < lut_size; i++) {
		printf("[%d] R:%4x G:%4x B:%4x\n", 
		       i, lut[i].red, lut[i].green, lut[i].blue);
	}
}


int main(int argc, char const *argv[])
{
	struct color3d coeffs[LUT_SIZE];
	struct _drm_color_lut lut[LUT_SIZE];

	load_table(coeffs, LUT_SIZE, 1);
	print_coeffs(coeffs, LUT_SIZE);

	coeffs_to_lut(coeffs, lut, LUT_SIZE);
	print_lut(lut, LUT_SIZE);
	return 0;
}