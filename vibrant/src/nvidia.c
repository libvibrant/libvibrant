#include "vibrant/nvidia.h"

#include <NVCtrl/NVCtrlLib.h>
#include <float.h>

double nvidia_get_saturation(Display *dpy, int id){
    int nv_saturation;
    double saturation;
    XNVCTRLQueryTargetAttribute(dpy, NV_CTRL_TARGET_TYPE_DISPLAY, id, 0, NV_CTRL_DIGITAL_VIBRANCE, &nv_saturation);

    if(nv_saturation < 0){
        saturation = (double)(nv_saturation+1024)/1024;
    }
    else{
        saturation = (double)(nv_saturation*3+1023)/1023;
    }

    return saturation;
}

void nvidia_set_saturation(Display *dpy, int id, double saturation){
    int nv_saturation;
    //is saturation roughly in [0.0, 1.0]
    if(saturation > 0.0 && saturation < 1.0+DBL_EPSILON){
        nv_saturation = saturation*1024-1024;
    }
    else{
        nv_saturation = (saturation*1023-1023)/3;
    }

    XNVCTRLSetTargetAttribute(dpy, NV_CTRL_TARGET_TYPE_DISPLAY, id, 0, NV_CTRL_DIGITAL_VIBRANCE, nv_saturation);
}
