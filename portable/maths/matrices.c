#include <stdarg.h>

#include <memory.h>
#include <string.h>
#include <maths.h>

addressCache_t matrixCache;

matrix_t phNewMatrix(uint8_t m, uint8_t n, ...) {
	va_list args;
	matrix_t mat;
	float f;
	mat.m = m;
	mat.n = n;

	mat.data = memalloc(mat.m * mat.n * sizeof(float));
	cacheAddress(&matrixCache, mat.data);

	va_start(args, n);
	for (size_t i = 0; i < mat.m * mat.n; i++) {
		mat.data[i] = (float)va_arg(args, double);
	}
	va_end(args);

	return mat;
}

matrix_t phCopyMatrix(matrix_t m) {
	matrix_t mat;
	mat.m = m.m;
	mat.n = m.n;

	mat.data = memalloc(mat.m * mat.n * sizeof(float));
	memcpy(mat.data, m.data, mat.m * mat.n * sizeof(float));
	cacheAddress(&matrixCache, mat.data);

	return mat;
}

void phSetMatrixElement(matrix_t *mat, int m, int n, double f) {
	mat->data[mat->n*m + n] = f;
}
// ``┌ w    0 0 0 ┐```
// ``│ 0    h 0 0 │```
// ``│ 0    0 1 0 │```
// ``└ x -y-h 0 1 ┘``


/**
 * 
 * ```text
 * mat = ┌ w    0 0 0 ┐
 *       │ 0    h 0 0 │
 *       │ 0    0 1 0 │
 *       └ x -y-h 0 1 ┘
 * ```
 *
 * @param mat pointer to matrix
 * @param rec rectangle data to insert
 */
void phSetMatrixToRec(matrix_t *mat, irec32_t rec) {
	float arr[4 * 4] = {
		rec.w,  0,           0,     0, // ┌ w    0 0 0 ┐
		0,      rec.h,       0,     0, // │ 0    h 0 0 │
		0,      0,           1,     0, // │ 0    0 1 0 │
		rec.x, -rec.y-rec.h, 0,     1, // └ x -y-h 0 1 ┘
	};

	for (size_t i = 0; i < 4*4; i++)
		mat->data[i] = arr[i];
}