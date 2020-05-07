#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#ifndef VIBRANT_VIBRANT_H
#define VIBRANT_VIBRANT_H
#ifdef __cplusplus
extern "C" {
#endif

/**
 * See get_saturation_ctm
 * @param dpy
 * @param output
 * @param x_status
 * @return
 */
double get_saturation(Display *dpy, RROutput output, int *x_status);

/**
 * See set_saturation_ctm
 * @param dpy
 * @param output
 * @param x_status
 * @return
 */
void set_saturation(Display *dpy, RROutput output, double saturation,
                    int *x_status);


#ifdef __cplusplus
}
#endif
#endif // VIBRANT_VIBRANT_H
