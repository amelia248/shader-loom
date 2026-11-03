#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <memory.h>
#include <maths.h>


// #define min(_a, _b) ((_a) < (_b) ? (_a) : (_b))
// #define max(_a, _b) ((_a) > (_b) ? (_a) : (_b))

size_t sizeofType_t(type_t t) {
	switch(t) {
		case PH_TYPE_FLOAT: return sizeof(float);
		case PH_TYPE_INT: return sizeof(int);
		default: return 0;
	}
}

void* memalloc(size_t size) {
	return malloc(size);
}
void memfree(void* memory) {
	free(memory);
}

void memcpy_blocks(void* dst, void* src, size_t count, size_t dstStride, size_t srcStride) {
	if (count == 0) return;
	size_t minSize = phMin_size_t(dstStride, srcStride);
	for (size_t i = 0; i < count; i++) {
		memcpy(dst + dstStride*i, src + srcStride*i, minSize);
	}
}
void memset_blocks(void* dst, uint8_t src, size_t count, size_t dstStride, size_t blockSize) {
	if (count == 0) return;
	size_t minSize = phMin_size_t(dstStride, blockSize);
	for (size_t i = 0; i < count; i++) {
		memset(dst + dstStride*i, src, minSize);
	}
}