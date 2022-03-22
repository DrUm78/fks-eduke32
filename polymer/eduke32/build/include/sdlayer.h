// SDL interface layer
// for the Build Engine
// by Jonathon Fowler (jonof@edgenetwk.com)

#ifndef __build_interface_layer__
#define __build_interface_layer__ SDL

#include "baselayer.h"

struct sdlappicon {
	int32_t width,height;
	uint32_t *pixels;
	uint8_t *mask;
};

enum screenscalemode_t
{
	SCREEN_Cropped,
	SCREEN_Scaled,
	SCREEN_Stretched
};

extern enum screenscalemode_t sscalemode;
extern int32_t screencropoffset;

#else
#if (__build_interface_layer__ != SDL)
#error "Already using the " __build_interface_layer__ ". Can't now use SDL."
#endif
#endif // __build_interface_layer__

