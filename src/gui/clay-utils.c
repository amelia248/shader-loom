#include <GLFW/glfw3.h>

#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>

#include <memory.h>
#include <maths.h>

#define PHOTON_FEATURE__OPENGL_STACK_OPERATIONS
#include "../photon.h"

#include <clay/clay.h>

Clay_Color phColourToClayColour(colour_t c) {
	return (Clay_Color){ .r=c.r, .g=c.g, .b=c.b, .a=c.a };
}
colour_t phClayColourToPhColour(Clay_Color c) {
	return (colour_t){ .r=c.r, .g=c.g, .b=c.b, .a=c.a };
}


Clay_Color phLerpClayColour(Clay_Color a, Clay_Color b, double t) {
	Clay_Color c;

	c.r = a.r + (b.r - a.r)*t;
	c.g = a.g + (b.g - a.g)*t;
	c.b = a.b + (b.b - a.b)*t;
	c.a = a.a + (b.a - a.a)*t;
	

	return c;
}