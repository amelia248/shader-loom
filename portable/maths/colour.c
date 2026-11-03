#include <maths.h>

colour_t phLerpColour(colour_t a, colour_t b, double t) {
	colour_t c;

	c.r = a.r + (b.r - a.r)*t;
	c.g = a.g + (b.g - a.g)*t;
	c.b = a.b + (b.b - a.b)*t;
	c.a = a.a + (b.a - a.a)*t;

	return c;
}

// if `mask = 0xaba0/0xabaf` then `out = (a.red, b.green, a.blue, lerp(a.alpha,b.alpha, t))
colour_t phMaskedLerpColour(colour_t a, colour_t b, double t, uint16_t mask) {
	colour_t c;
#define rep(_shift, _channel)                                                              \
	if      ((mask>>_shift) & 0xf == 0xa) c._channel = a._channel;                            \
	else if ((mask>>_shift) & 0xf == 0xb) c._channel = b._channel;                             \
	else                                  c._channel = a._channel + (b._channel - a._channel)*t;\

	rep(16, r)
	rep( 8, g)
	rep( 4, b)
	rep( 0, a)
	return c;
}