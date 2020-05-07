//
// Created by scrumplex on 06.05.20.
//

#include <stdint.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#ifndef VIBRANT_CTM_H
#define VIBRANT_CTM_H

/**
 * Set a DRM blob property on the given output. It calls XSync at the end to
 * flush the change request so that it applies.
 *
 * Return values:
 *   - BadAtom if the given name string doesn't exist
 *   - BadName if the property referenced by the name string does not exist
 *   - Success if everything went well
 *
 * @param dpy The X Display
 * @param output RandR output to set the property on
 * @param prop_name String name of the property
 * @param blob_data The data of the property blob
 * @param blob_bytes Size of the data, in bytes
 * @return X-defined return code
 */
int set_output_blob(Display *dpy, RROutput output, const char *prop_name,
                    void *blob_data, size_t blob_bytes);

/**
 * Get a DRM blob property on the given output.
 *
 * This method is heavily biased against CTM values. It may not work as is for
 * other properties.
 *
 * Return values:
 *   - BadAtom if the given name string doesn't exist
 *   - BadName if the property referenced by the name string does not exist
 *   - Success if everything went well
 *
 * @param dpy The X Display
 * @param output RandR output to set the property on
 * @param prop_name String name of the property
 * @param blob_data The data of the property blob. The output will be put here.
 * @return X-defined return code
 */
int get_output_blob(Display *dpy, RROutput output, const char *prop_name,
                    uint64_t *blob_data);

/**
 * Create a DRM color transform matrix using the given coefficients, and set
 * the output's CRTC to use it
 *
 * @param dpy The X Display
 * @param output RandR output to set the property on
 * @param coeffs double array of size 9 containing the coefficients for CTM
 * @return X-defined return code (See set_output_blob())
 */
int set_ctm(Display *dpy, RROutput output, double *coeffs);

/**
 * Query current CTM values from output's CRTC and convert them to double
 * coefficients.
 *
 * @param dpy The X Display
 * @param output RandR output to set the property on
 * @param coeffs double array of size 9. Will hold the coefficients.
 * @return X-defined return code (See get_output_blob())
 */
static int get_ctm(Display *dpy, RROutput output, double *coeffs);

/**
 * Get saturation of output in human readable format.
 * (See saturation_to_coeffs() doc)
 *
 * @param dpy The X Display
 * @param output RandR output to get the saturation from
 * @param x_status X-defined return code (See get_ctm())
 * @return Saturation of output
 */
double get_saturation_ctm(Display *dpy, RROutput output, int *x_status);

/**
 * Get saturation of output in human readable format.
 * (See saturation_to_coeffs() doc)
 *
 * @param dpy The X Display
 * @param output RandR output to set the saturation on
 * @param saturation Saturation of output
 * @param x_status X-defined return code (See get_ctm())
 */
void set_saturation_ctm(Display *dpy, RROutput output, double saturation,
                        int *x_status);

/**
 * Check if output has the CTM property.
 *
 * @param dpy The X Display
 * @param output RandR output to get the information from
 * @return 1 if it has a property, 0 if it doesn't or X doesn't support it
 */
int output_has_ctm(Display *dpy, RROutput output);

#endif //VIBRANTX_CTM_H
