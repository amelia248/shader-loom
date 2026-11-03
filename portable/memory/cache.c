#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <memory.h>

addressCache_t newCache(size_t len) {
	addressCache_t a;
	
	const size_t addrSize = len * sizeof(void*);
	const size_t gapsSize = (len + 7) / 8;
	const void*  addr     = memalloc(addrSize + gapsSize);

	a.addresses = (void**)(addr);
	a.cap       = len;
	// a.len       = 0;
	a.gaps      = (uint8_t*)(addr + addrSize);

	memset(a.gaps, 0, gapsSize);

	return a;
}

void delCache(addressCache_t* a) {
	memfree(a->addresses); a->addresses = NULL;
	a->gaps = NULL;
}
// returns the index the address was stored at
int cacheAddress(addressCache_t* a, void *addr) {
	for (size_t i = 0; i < a->cap; i++) {

		if (a->gaps[i/8] == 0xff) i += 8;

		if ((a->gaps[i/8] >> (i%8) & 1) == 0) {
			a->addresses[i] = addr;
			a->gaps[i/8] |= 1 << (i%8);
			return i;
		}
	}
	return -1;
}
// returns 1 (found address) or 0 (didn't find address)
int removeAddress(addressCache_t* a, void *addr) {
	for (size_t i = 0; i < a->cap; i++) {
		if ((a->gaps[i/8] >> (i%8) & 1) == 1 && a->addresses[i] == addr) {
			a->gaps[i/8] &= ~(1 << (i%8));

			return 1;
		}
	}
	return 0;
}

void clearCache(addressCache_t* a) {
	for (size_t i = 0; i < a->cap; i++) {
		if ((a->gaps[i/8] >> (i%8) & 1) == 1) {
			memfree(a->addresses[i]);
		}
	}

	memset(a->gaps, 0, (a->cap + 7) / 8);
}

int nextFreeCacheIndex(addressCache_t* a) {
	for (size_t i = 0; i < a->cap; i++) {

		if (a->gaps[i/8] == 0xff) i += 8;

		if ((a->gaps[i/8] >> (i%8) & 1) == 0) {
			// a->addresses[i] = addr;
			// a->gaps[i/8] |= 1 << (i%8);

			return i;
		}
	}
	return -1;
}

void* getAddress(addressCache_t* a, size_t index) {
	if (index >= a->cap) return nullptr;

	return a->addresses[index];
}
// void foreachAddress(addressCache_t* a, void (*func)(void*)) {
// 	// for (size_t i = 0; i < a->cap; i++)
// 	// 	if ((a->gaps[i/8] >> (i%8) & 1) == 1) {
// 	// 		func(a->addresses[i]);
// 	// 	}
// 	addressCache_t _a = *a;
// 	for (size_t _i=0, void *_addr; _i<_a.cap, _addr=a.addresses[_i]; i++) {
		
// 	}
// }