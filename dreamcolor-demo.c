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

#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>


#define LUT_SIZE 16


/**
 * The below data structures are identical to the ones used by DRM. They are
 * here to help us structure the data being passed to the kernel.
 */
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

/* Struct to make constructing luts easier. */
struct color3d {
	double r;
	double g;
	double b;
};


/*******************************************************************************
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

static int fd_is_device(int fd, const char *expect)
{
	char name[5] = "";
	drm_version_t version = {0};

	version.name_len = 4;
	version.name = name;
	if (drmIoctl(fd, DRM_IOCTL_VERSION, &version))
		return 0;

	return strcmp(expect, version.name) == 0;
}

static int open_drm_device()
{
	char *base = "/dev/dri/card";
	int i = 0;
	for (i = 0; i < 16; i++) {
		char name[80];
		int fd;

		sprintf(name, "%s%u", base, i);
		printf("Opening %s... ", name);
		fd = open(name, O_RDWR);
		if (fd == -1) {
			printf("Failed.\n");
			continue;
		}

		if (fd_is_device(fd, "i915")){
			printf("Success!\n");
			return fd;
		}

		printf("Not an amdgpu device.\n");
		close(fd);
	}
	return -1;
}

/*******************************************************************************
 * main
 */

int main(int argc, char const *argv[])
{
	struct color3d coeffs[LUT_SIZE];
	struct _drm_color_lut lut[LUT_SIZE];

	int ret;

	int fd = open_drm_device();
	if (fd == -1) {
		printf("No valid devices found\n");
		return -1;
	}

	load_table(coeffs, LUT_SIZE, 1);
	coeffs_to_lut(coeffs, lut, LUT_SIZE);

	size_t size = sizeof(struct _drm_color_lut) * LUT_SIZE;
	uint32_t blob_id = 0;

	ret = drmModeCreatePropertyBlob(fd, lut, size, &blob_id);
	if (ret) {
		printf("Failed to create blob. %d\n", ret);
		return ret;
	}

	printf("Successfully created property blob with id %d\n", blob_id);
	drmModePropertyBlobPtr blob = drmModeGetPropertyBlob(fd, blob_id);
	if (!blob) {
		printf("Failed to get blob.\n");
		return -1;
	}

	struct _drm_color_lut *ret_lut = (struct _drm_color_lut *)blob->data;

	print_lut(ret_lut, LUT_SIZE);

	ret = drmModeDestroyPropertyBlob(fd, blob_id);
	if (ret) {
		printf("Failed to destroy blob. %d\n", ret);
		return -1;
	}

	drmModeFreePropertyBlob(blob);

	printf("Done!\n");
	return 0;
}