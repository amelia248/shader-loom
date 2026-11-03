#ifndef PHOTON_PORTABLE_MATHS_H
#define PHOTON_PORTABLE_MATHS_H

#include <stdint.h>

#pragma region MATHS.C

typedef struct { uint8_t  x,y,w,h; } urec8_t;
typedef struct {  int8_t  x,y,w,h; } irec8_t;
typedef struct {  int32_t x,y,w,h; } irec32_t;
typedef struct {    float x,y,w,h; } frec_t;

typedef struct { float  x, y; } fvec2_t;
typedef struct { double x, y; } dvec2_t;
typedef struct { int    x, y; } ivec2_t;

typedef struct { float x, y, z, w; } fvec4_t;
typedef struct { int   x, y, z, w; } ivec4_t;

double phRound(double x, double toNearest);
int    phMin_int(int a, int b);
int    phMax_int(int a, int b);
double phMin_double(double a, double b);
double phMax_double(double a, double b);
size_t phMin_size_t(size_t a, size_t b);
size_t phMax_size_t(size_t a, size_t b);

int    phClamp_int(int x, int min, int max);
double phClamp_double(double x, double min, double max);

uint32_t phSwapEndianness(uint32_t x);

#pragma endregion

#pragma region COLOUR.C

// 0xrrggbbaa
typedef union {
	uint32_t value;
	struct {
		uint8_t a, b, g, r;
	};
} colour_t;

// `rgb(0xf18)      -> (colour){0xff1188ff}`
// `rgb(0xf18a)     -> (colour){0xff1188aa}`
// `rgb(0xf92672)   -> (colour){0xf92672ff}`
// `rgb(0xf92672ab) -> (colour){0xf92672ab}`
#define rgb(_hex) (          \
	(sizeof(#_hex) - 3) == 3 ? (\
		(colour_t){                      \
			.r = ((_hex & 0xf00) >> 8)*0x11,\
			.g = ((_hex & 0x0f0) >> 4)*0x11, \
			.b =  (_hex & 0x00f)      *0x11,  \
			.a =  0xff,                        \
		}                                     \
	):(                        \
	(sizeof(#_hex) - 3) == 4 ? (\
		(colour_t){                        \
			.r = ((_hex & 0xf000) >> 12)*0x11,\
			.g = ((_hex & 0x0f00) >>  8)*0x11, \
			.b = ((_hex & 0x00f0) >>  4)*0x11,  \
			.a =  (_hex & 0x000f)       *0x11,   \
		}                                       \
	):(                        \
	(sizeof(#_hex) - 3) == 6 ? (\
		(colour_t){.value = (_hex << 8) | 0xff,}\
	):(\
		(colour_t){.value = _hex}\
	)))\
)

// #define clayRGB(_hex) ((struct Clay_Color {float r, g, b, a;}){255,0,128,255});

colour_t phLerpColour(colour_t a, colour_t b, double t);
colour_t phMaskedLerpColour(colour_t a, colour_t b, double t, uint16_t mask);

#pragma endregion

#pragma region MATRICES.C

typedef struct {
	float *data;

	uint8_t m;
	uint8_t n;
} matrix_t;

matrix_t phNewMatrix(uint8_t m, uint8_t n, ...);
matrix_t phCopyMatrix(matrix_t m);

void phSetMatrixElement(matrix_t *mat, int m, int n, double f);
void phSetMatrixToRec(matrix_t *mat, irec32_t rec);

#pragma endregion

#endif