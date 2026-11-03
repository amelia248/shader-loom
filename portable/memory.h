#ifndef PHOTON_PORTABLE_MEMORY_H
#define PHOTON_PORTABLE_MEMORY_H

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>



#pragma region MEMORY.C

typedef enum {
	PH_TYPE_FLOAT,
	PH_TYPE_INT,
} type_t;

size_t sizeofType_t(type_t t);

void* memalloc(size_t size);
void memfree(void* memory);

void memcpy_blocks(void* dst, void* src, size_t count, size_t dstStride, size_t srcStride);
void memset_blocks(void* dst, uint8_t src, size_t count, size_t dstStride, size_t blockSize);

#pragma endregion

#pragma region STRINGS.C

typedef struct {
	size_t len;
	char  *buf;
} string_t;

typedef const char* c_string_t;

#define null_string ((string_t){.buf=nullptr, .len=0})
#define newstring_t(_b, _l) ((string_t){.buf=(_b), .len=(_l)})

string_t newstring(c_string_t buf);
string_t newstring_len(size_t len);
string_t newstring_substr(c_string_t buf, size_t maxLen);
void     delstring(string_t s);

void stringcat_buf(string_t *s, c_string_t source);
void stringcat_str(string_t* s, string_t source);
string_t newstring_readFile(c_string_t path);
#pragma endregion

#pragma region CACHE.C
// `[addresses][gaps list]`
typedef struct {
	void   **addresses;
	size_t   cap;
	uint8_t *gaps;
} addressCache_t;

addressCache_t newCache(size_t len);
void           delCache(addressCache_t* a);
int   cacheAddress(addressCache_t* a, void *addr);
int   removeAddress(addressCache_t* a, void *addr);
void  clearCache(addressCache_t* a);
int   nextFreeCacheIndex(addressCache_t* a);
void* getAddress(addressCache_t* a, size_t index);
// void foreachAddress(addressCache_t* a, void (*func)(void*));

#define foreachAddress(_a, _i)   \
for (size_t _i=0; _i<_a.cap; _i++)\
	if ((_a.gaps[_i/8] >> (_i%8) & 1) == 1)
#pragma endregion


#pragma region STACK.C
typedef struct {
	void  **arr;
	ssize_t cap;
	ssize_t top;
} addressStack_t;

addressStack_t newStack(size_t len);
void           delStack(addressStack_t* a);
void  push(addressStack_t* a, void* addr);
void *pop (addressStack_t* a);
void *peek(addressStack_t* a);
void  poke(addressStack_t* a, void* new);
void  swap(addressStack_t* a, void* old, void* new);
#pragma endregion

/*
#pragma region HASHMAP.C
typedef struct {
	struct {
		void* addr;
		c_string_t key;
	}* values;
	size_t   cap;
	uint8_t *gaps;
	
} hashmap_t;

hashmap_t newHashmap(size_t len);
void      delHashmap(hashmap_t* a);
#pragma endregion
*/

/*
#pragma region MIXED_STACK.C
typedef struct {
	struct {
		void *addr;
		size_t previous;
	} *values;
	hashmap_t map;
} mixedAddressStack_t;

mixedAddressStack_t newMixedStack(size_t valuesLen, size_t hashmapLen);
void                delMixedStack(mixedAddressStack_t* a);
void  pushMixed(mixedAddressStack_t* a, c_string_t key, void* addr);
void *popMixed (mixedAddressStack_t* a, c_string_t key);
void *peekMixed(mixedAddressStack_t* a, c_string_t key);
void  pokeMixed(mixedAddressStack_t* a, c_string_t key, void* new);
void  swapMixed(mixedAddressStack_t* a, c_string_t key, void* old, void* new);
#pragma endregion
*/

#endif