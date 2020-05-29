//user code should not include this directly, use the interface provided by vibrant.h

#ifndef LIBVIBRANT_NVIDIA_H
#define LIBVIBRANT_NVIDIA_H

#include <X11/Xlib.h>

double nvidia_get_saturation(Display *dpy, int id);

void nvidia_set_saturation(Display *dpy, int id, double saturation);

#endif //LIBVIBRANT_NVIDIA_H
