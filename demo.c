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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>


#define LUT_SIZE 4096

#define PROP_GAMMA "GAMMA_LUT"
#define PROP_DEGAMMA "DEGAMMA_LUT"
#define PROP_CTM "CTM"

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

static int __fd_is_device(int fd, const char *expect)
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

		if (__fd_is_device(fd, "amdg")){
			printf("Success!\n");
			return fd;
		}

		printf("Not an amdgpu device.\n");
		close(fd);
	}
	return -1;
}

/**
 * Translate coefficients to a color LUT format that DRM accepts.
 * @coeffs: Input coefficients
 * @lut: DRM LUT struct, used to create the blob. The translated values will be
 *       placed here.
 * @lut_size: Number of entries in the LUT.
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
static void coeffs_to_ctm(double *coeffs,
			  struct _drm_color_ctm *ctm)
{
	int i;
	for (i = 0; i < 9; i++) {
		if (coeffs[i] < 0) {
			ctm->matrix[i] =
				(int64_t) (-coeffs[i] * ((int64_t) 1L << 32));
			ctm->matrix[i] |= 1ULL << 63;
		} else
			ctm->matrix[i] =
				(int64_t) (coeffs[i] * ((int64_t) 1L << 32));
	}
}

/**
 * The three functions below contain three different methods of generating a
 * gamma LUT. They all output it in the intermediary coefficient format.
 *
 * Call coeffs_to_lut on the coefficients to translate them.
 */
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

static void load_table(struct color3d *coeffs, int lut_size, double *exps)
{
	int i;
	double sanitized_exps[3];

	double min = 1.0 / (1 << 10);

	/* Ensure no zero exps. */
	for (i = 0; i < 3; i++) {
		if (exps[i] < min)
			sanitized_exps[i] = min;
		else
			sanitized_exps[i] = exps[i];
	}

	for (i = 0; i < lut_size; i++) {
		coeffs[i].r = pow((double) i * 1.0 / (double) (lut_size - 1),
				   1.0 / sanitized_exps[0]);
		coeffs[i].g = pow((double) i * 1.0 / (double) (lut_size - 1),
				   1.0 / sanitized_exps[1]);
		coeffs[i].b = pow((double) i * 1.0 / (double) (lut_size - 1),
				   1.0 / sanitized_exps[2]);
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
				    const char *name)
{
	int i;
	RROutput ret;
	XRROutputInfo *output_info;

	for (i = 0; i < res->noutput; i++) {
		ret = res->outputs[i];
		output_info = XRRGetOutputInfo (dpy, res, ret);

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
 * @blob_id: The new blob_id to set this blob property to.
 *
 * Return: X-defined return codes:
 *     - BadAtom if the given name string doesn't exist.
 *     - BadName if the property referenced by the name string does not exist on
 *       the given connector
 *     - Success otherwise.
 */
static int set_output_blob_id(Display *dpy, RROutput output,
			      const char *prop_name,
			      uint32_t blob_id)
{
	Atom prop_atom;
	XRRPropertyInfo *prop_info;

	/* Find the X Atom associated with the property name */
	prop_atom = XInternAtom (dpy, prop_name, 1);
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

	/* Change the property, then call XSync to apply it. 
	 *
	 * Gotcha: Do not release the DRM blob attached to the blob ID before
	 * the property change is synced! Else the blob will be freed in kernel
	 * before it can be set.
	 */
	printf("Set property '%s' to %d\n", prop_name, blob_id);
	XRRChangeOutputProperty(dpy, output, prop_atom,
				XA_INTEGER, 32, PropModeReplace,
				(unsigned char *) &blob_id, 1);
	XSync(dpy, 0);

	return Success;
}

/**
 * Create a DRM color LUT blob using the given coefficients, and set the
 * output's CRTC to use it. Since setting degamma and regamma follows similar
 * procedures, a flag is used to determine which one is set. Also note the
 * special case of setting SRGB gamma, explained further below.
 *
 * @drm_fd: The file descriptor of the DRM interface.
 * @coeffs: Coefficients used to create the DRM color LUT blob.
 * @is_srgb: True if SRGB gamma is being programmed. This is a special case,
 *           since amdgpu DC defaults to SRGB when no DRM blob (i.e. NULL) is
 *           set. In other words, there is no need to create a blob (just set
 *           the blob id to 0)
 * @is_degamma: True if degamma is being set. Set regamma otherwise.
 */
static int set_gamma(Display *dpy, RROutput output, int drm_fd,
		     struct color3d *coeffs, int is_srgb, int is_degamma)
{
	struct _drm_color_lut lut[LUT_SIZE];
	uint32_t blob_id = 0;

	int ret = 0;

	if (!is_srgb) {
		/* Using LUT */
		coeffs_to_lut(coeffs, lut, LUT_SIZE);
		size_t size = sizeof(struct _drm_color_lut) * LUT_SIZE;
		ret = drmModeCreatePropertyBlob(drm_fd, lut, size, &blob_id);
		if (ret) {
			printf("Failed to create blob. %d\n", ret);
			return ret;
		}
	}
	/* Else:
	 * In the special case of SRGB, don't create the blob. We just need to
	 * set a NULL blob id (0) */

	ret = set_output_blob_id(dpy, output,
				 is_degamma ? PROP_DEGAMMA : PROP_GAMMA,
				 blob_id);

	if (blob_id) {
		/* Make sure to destroy the blob if one was created.
		 *
		 * Note that we can destroy the blob immediately after it's set.
		 * The blob property is ref-counted within the kernel, and will
		 * be freed once the CRTC it's attached on is destroyed.
		 */
		drmModeDestroyPropertyBlob(drm_fd, blob_id);
	}

	return ret;
}

/**
 * Create a DRM color LUT blob using the given coefficients, and set the
 * output's CRTC to use it.
 *
 * The process is similar to set_gamma(). The only difference is the type of
 * blob being created. See set_gamma() for a description of the steps being
 * done.
 */
static int set_ctm(Display *dpy, RROutput output, int drm_fd, double *coeffs)
{
	struct _drm_color_ctm ctm;
	uint32_t blob_id = 0;
	size_t blob_size;

	int ret;

	coeffs_to_ctm(coeffs, &ctm);
	blob_size = sizeof(ctm);

	ret = drmModeCreatePropertyBlob(drm_fd, &ctm, blob_size, &blob_id);
	if (ret) {
		printf("Failed to create blob. %d\n", ret);
		return ret;
	}

	ret = set_output_blob_id(dpy, output,
				 PROP_CTM, blob_id);

	drmModeDestroyPropertyBlob(drm_fd, blob_id);

	return ret;
}

/*******************************************************************************
 * main function, and functions to assist in parsing input.
 */

/**
 * Parse a list of doubles from the given string. ':' is used as the delimiter.
 *
 * @str: String containing doubles, delimited by ':'
 * @count: Number of doubles expected to be found in the string.
 *
 * Return: an array of doubles. If the expected number of doubles is not met,
 * return NULL.
 */
static double *parse_d(const char *str, int count)
{
	char *token;
	uint32_t len = strlen(str);
	char *cpy_str;
	char * const cpy_str_head = malloc(sizeof(char) * len);
	double *ret = malloc(sizeof(double) * count);

	cpy_str = cpy_str_head;
	strcpy(cpy_str, str);

	int i;
	for (i = 0; i < count; i++) {
		token = strsep(&cpy_str, ":");
		if (!token)
			return NULL;
		ret[i] = strtod(token, NULL);
	}

	free(cpy_str_head);
	return ret;
}

/**
 * Parse user input, and fill the coefficients array with the requested LUT.
 * Degamma currently only supports srgb, or linear tables.
 *
 * @gamma_opt: User input
 * @coeffs: Array of color3d structs. The requested LUT will be filled in here.
 * @is_srgb: Will be set to true if user requested SRGB LUT.
 *
 * Return: True if user has requested gamma change. False otherwise.
 */
int parse_user_degamma(char *gamma_opt, struct color3d *coeffs, int *is_srgb)
{
	double linear_exps[3] = { 1.0, 1.0, 1.0 };

	*is_srgb = 0;

	if (!gamma_opt)
		return 0;

	if (!strcmp(gamma_opt, "srgb")) {
		printf("Using srgb degamma curve\n");
		*is_srgb = 1;
		return 1;
	}
	if (!strcmp(gamma_opt, "linear")) {
		printf("Using linear degamma curve\n");
		load_table(coeffs, LUT_SIZE, linear_exps);
		return 1;
	}

	printf("Degamma only supports 'srgb', or 'linear' LUT. Skipping.\n");
	return 0;
}

/**
 * Parse user input, and fill the coefficients array with the requested CTM.
 *
 * @ctm_opt: user input
 * @coeffs: Array of 9 doubles. The requested CTM will be filled in here.
 *
 * Return: True if user has requested CTM change. False otherwise.
 */
int parse_user_ctm(char *ctm_opt, double *coeffs)
{
	if (!ctm_opt)
		return 0;

	if (!strcmp(ctm_opt, "id")) {
		printf("Using identity CTM\n");
		double temp[9] = {
			1, 0, 0,
			0, 1, 0,
			0, 0, 1
		};
		memcpy(coeffs, temp, sizeof(double) * 9);
		return 1;
	}
	/* CTM is left-multiplied with the input color vector */
	if (!strcmp(ctm_opt, "rg")) {
		printf("Using red-to-green CTM\n");
		double temp[9] = {
			0, 0, 0,
			1, 1, 0,
			0, 0, 1
		};
		memcpy(coeffs, temp, sizeof(double) * 9);
		return 1;
	}
	if (!strcmp(ctm_opt, "rb")) {
		printf("Using red-to-blue CTM\n");
		double temp[9] = {
			0, 0, 0,
			0, 1, 0,
			1, 0, 1
		};
		memcpy(coeffs, temp, sizeof(double) * 9);
		return 1;
	}

	double *temp = parse_d(ctm_opt, 9);
	if (!temp) {
		printf("%s is not a valid CTM. Skipping.\n",
		       ctm_opt);
		return 0;
	}

	printf("Using custom CTM:\n");
	printf("    %2.4f:%2.4f:%2.4f\n", temp[0], temp[1], temp[2]);
	printf("    %2.4f:%2.4f:%2.4f\n", temp[3], temp[4], temp[5]);
	printf("    %2.4f:%2.4f:%2.4f\n", temp[6], temp[7], temp[8]);

	memcpy(coeffs, temp, sizeof(double) * 9);
	free(temp);
	return 1;

}

/**
 * Parse user input, and fill the coefficients array with the requested LUT.
 * If predefined SRGB LUT is requested, the coefficients array is not touched,
 * and is_srgb is set to true. See set_gamma() for why.
 *
 * @gamma_opt: User input
 * @coeffs: Array of color3d structs. The requested LUT will be filled in here.
 * @is_srgb: Will be set to true if user requested SRGB LUT.
 *
 * Return: True if user has requested gamma change. False otherwise.
 */
int parse_user_regamma(char *gamma_opt, struct color3d *coeffs, int *is_srgb)
{
	*is_srgb = 0;

	if (!gamma_opt)
		return 0;

	if (!strcmp(gamma_opt, "max")) {
		/* Use max gamma curve */
		printf("Using max regamma curve\n");
		load_table_max(coeffs, LUT_SIZE);
		return 1;
	}
	if (!strcmp(gamma_opt, "min")) {
		/* Use min gamma curve */
		printf("Using zero regamma curve\n");
		load_table_zero(coeffs, LUT_SIZE);
		return 1;
	}
	if (!strcmp(gamma_opt, "srgb")) {
		printf("Using srgb regamma curve\n");
		*is_srgb = 1;
		return 1;
	}

	/* Custom exponential curve */
	double *exps = parse_d(gamma_opt, 3);
	if (!exps) {
		printf("%s is not a valid regamma exponent triple. Skipping.\n",
		       gamma_opt);
		return 0;
	}
	printf("Using custom regamma curve %.4f:%.4f:%.4f\n",
	       exps[0], exps[1], exps[2]);
	load_table(coeffs, LUT_SIZE, exps);
	free(exps);
	return 1;
}

static char HELP_STR[] = { 
	#include "help.xxd"
};

static void print_short_help()
{
	/* Just get first line, up to the first new line.*/
	uint32_t len = strlen(HELP_STR);
	char *short_help;

	int i;
	for (i = 0; i < len; i++) {
		if (HELP_STR[i] == '\n')
			break;
	}

	short_help = malloc(sizeof(char) * i);
	strncpy(short_help, HELP_STR, i);

	printf("%s\n", short_help);
	free(short_help);
}



int main(int argc, char *const argv[])
{
	/* These coefficient arrays store an intermediary form of the DRM blob
	 * to be set. They will be translated into the format that DRM expects
	 * when they are given to libdrm to be created within the kernel.
	 */
	struct color3d degamma_coeffs[LUT_SIZE];
	double ctm_coeffs[9];
	struct color3d regamma_coeffs[LUT_SIZE];

	int drm_fd;
	int ret = 0;

	/* Things needed by xrandr to change output properties */
	Display *dpy;
	Window root;
	XRRScreenResources *res;
	RROutput output;


	/*
	 * Parse arguments
	 */
	int opt = -1;
	char *degamma_opt = NULL;
	char *ctm_opt = NULL;
	char *regamma_opt = NULL;
	char *output_name = NULL;

	int degamma_changed, degamma_is_srgb;
	int ctm_changed;
	int regamma_changed, regamma_is_srgb;

	while ((opt = getopt(argc, argv, "ho:d:c:r:")) != -1) {
		if (opt == 'd')
			degamma_opt = optarg;
		else if (opt == 'c')
			ctm_opt = optarg;
		else if (opt == 'r')
			regamma_opt = optarg;
		else if (opt == 'o')
			output_name = optarg;
		else if (opt == 'h') {
			printf("%s", HELP_STR);
			return 0;
		}
		else {
			print_short_help();
			return 1;
		}
	}

	/* Check that output is given */
	if (!output_name) {
		print_short_help();
		return 1;
	}

	/* Parse the input, and generate the intermediate coefficient arrays */
	degamma_changed = parse_user_degamma(degamma_opt, degamma_coeffs,
					     &degamma_is_srgb);
	ctm_changed = parse_user_ctm(ctm_opt, ctm_coeffs);
	regamma_changed = parse_user_regamma(regamma_opt, regamma_coeffs,
					     &regamma_is_srgb);

	/* Print help if input is not as expected */
	if (!degamma_changed && !ctm_changed && !regamma_changed) {
		print_short_help();
		return 1;
	}

	/* Open DRM device, and obtain file descriptor */
	drm_fd = open_drm_device();
	if (drm_fd == -1) {
		printf("No valid devices found\n");
		printf("Did you run with admin privilege?\n");
		return 1;
	}

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
		goto done;
	}

	/* Set the properties as parsed. The set_* functions will also
	 * translate the coefficients. */
	if (degamma_changed) {
		ret = set_gamma(dpy, output, drm_fd,
				degamma_coeffs, degamma_is_srgb, 1);
		if (ret)
			goto done;
	}
	if (ctm_changed) {
		ret = set_ctm(dpy, output, drm_fd, ctm_coeffs);
		if (ret)
			goto done;
	}
	if (regamma_changed) {
		ret = set_gamma(dpy, output, drm_fd,
				regamma_coeffs, regamma_is_srgb, 0);
		if (ret)
			goto done;
	}

done:
	/* Ensure proper cleanup */
	XRRFreeScreenResources(res);
	XCloseDisplay(dpy);
	close(drm_fd);

	return ret;
}
