#include <math.h>

#include <maths.h>

double phRound(double x, double toNearest) {
	return round(x / toNearest) * toNearest;
}

int    phMin_int(int a, int b) { return a < b ? a : b; }
int    phMax_int(int a, int b) { return a > b ? a : b; }
double phMin_double(double a, double b) { return a < b ? a : b; }
double phMax_double(double a, double b) { return a > b ? a : b; }
size_t phMin_size_t(size_t a, size_t b) { return a < b ? a : b; }
size_t phMax_size_t(size_t a, size_t b) { return a > b ? a : b; }

int phClamp_int(int x, int min, int max) {
	if (x < min) return min;
	if (x > max) return max;
	return x;
}

double phClamp_double(double x, double min, double max) {
	if (x < min) return min;
	if (x > max) return max;
	return x;
}

uint32_t phSwapEndianness(uint32_t x) {
	return
	((x >> 24) & 0x00'00'00'ff) |
	((x >>  8) & 0x00'00'ff'00) |
	((x <<  8) & 0x00'ff'00'00) |
	((x << 24) & 0xff'00'00'00) ;
}