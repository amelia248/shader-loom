#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <memory.h>

addressStack_t newStack(size_t len) {
	addressStack_t a;
	const void*  addr = memalloc(len * sizeof(void*));

	a.arr = (void**)(addr);
	a.cap = len;
	a.top = -1;

	return a;
}

void delStack(addressStack_t* a) {
	memfree(a->arr);
	a->arr = nullptr;
}

// push an address onto the stack
void push(addressStack_t* a, void* addr) {
	if (a->top >= a->cap) {
		printf("Stack Overflow\n");
		return;
	}
	a->arr[++a->top] = addr;
}

// pop & return an address off the stack
void *pop(addressStack_t* a) {
	if (a->top <= -1) {
		// printf("Stack UnderFlow\n");
		return nullptr;
	}
	return a->arr[a->top--];
}

// return the address at the top of the stack
void *peek(addressStack_t* a) {
	if (a->top <= -1) {
		// printf("Stack UnderFlow\n");
		return nullptr;
	}
	return a->arr[a->top];
}

// replace the address at the top of the stack
void poke(addressStack_t* a, void* new) {
	a->arr[a->top] = new;
}

// replace all occurrences of `old` with `new` in the stack
void swap(addressStack_t* a, void* old, void* new) {
	if (a->top <= -1) return;
	for (size_t i = 0; i <= a->top; i++) {
		if (a->arr[i] == old)
			a->arr[i] = new;
	}
}