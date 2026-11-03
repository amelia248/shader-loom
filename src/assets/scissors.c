#ifdef INTELLISENSE
	#define GLAPI
	#define APIENTRY
	#define GL_GLEXT_PROTOTYPES
	#include <glad/glcorearb.h>
#endif
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stddef.h>
#include <stdarg.h>

#include <memory.h>
#include <maths.h>

#include "../photon.h"

addressStack_t scissorStack;

void phGLPushScissor(scissor_t s) {
	scissor_t* ptr = memalloc(sizeof(s));
	*ptr = s;
	
	phGLSetScissor(s);

	push(&scissorStack, ptr);
}
scissor_t phGLPopScissor() {
	scissor_t* ptr = pop(&scissorStack);
	scissor_t s = *ptr;
	glScissor(s.x, phGetWinHeight() - (s.y+s.h), s.w, s.h);

	memfree(ptr);
	return s;
}
scissor_t phGLPeekScissor() {
	return *(scissor_t*)peek(&scissorStack);
}

void phGLSetScissor(scissor_t s) {
	glScissor(s.x, phGetWinHeight() - (s.y+s.h), s.w, s.h);
}