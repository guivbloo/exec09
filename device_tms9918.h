#ifndef DEVICE_TMS9918_H
#define DEVICE_TMS9918_H

#include "cimgui.h"

struct hw_device* tms9918_create (unsigned long size);
void tms9918_display (struct hw_device *dev, ImVec2 pos);
void tms9918_display_init(void) ;


#endif
