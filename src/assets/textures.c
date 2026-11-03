#ifdef INTELLISENSE
	#define GLAPI
	#define APIENTRY
	#define GL_GLEXT_PROTOTYPES
	#include <glad/glcorearb.h>
#endif
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "STB/stb-image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "STB/stb-write-image.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>

#include <memory.h>
#include <maths.h>

#include "../photon.h"

addressCache_t textureCache;
addressStack_t textureStack[PH_TEXTURE_GL_STACK_SLOTS_LEN];


void phGLPushTexture(int glSlot, texture_ptr t) {
	push(&textureStack[glSlot], t);
	glActiveTexture(GL_TEXTURE0 + glSlot);
	glBindTexture(GL_TEXTURE_2D, t->texture);
	glActiveTexture(0);
}
texture_ptr phGLPopTexture(int glSlot) {
	texture_ptr ret = pop(&textureStack[glSlot]);
	texture_ptr t   = peek(&textureStack[glSlot]);
	
	glActiveTexture(GL_TEXTURE0 + glSlot);
	glBindTexture(GL_TEXTURE_2D, t == NULL ? 0 : t->texture);
	glActiveTexture(0);
	return ret;
}
texture_ptr phGLPeekTexture(int glSlot) { return peek(&textureStack[glSlot]); }

/*
if `type` is dynamic, first procedural argument is a boolean for whether to reallocate the provided data to ensure it's permanent
*/
texture_ptr phNewTexture(uint8_t* data, int width, int height, textureFormat_e format, int filter, textureType_e type, ...) {
	texture_t t;
	GLenum glInternalFormat, glFormat;
	bool reallocData = false;

	va_list args;
	va_start(args, type);
	if (type == PH_TEXTURE_TYPE_DYNAMIC) {
		bool reallocData = va_arg(args, int);
	}
	va_end(args);


	t.width  = width;
	t.height = height;
	t.filter = filter;
	t.type   = type;
	t.index  = nextFreeCacheIndex(&textureCache);

	glGenTextures(1, &t.texture);
	glActiveTexture(GL_TEXTURE0 + t.index);
	glBindTexture(GL_TEXTURE_2D, t.texture);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, t.filter);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	switch (format) {
		case PH_TEXTURE_FORMAT_R:    {
			glInternalFormat = GL_R8;
			glFormat = GL_RED;
			GLint swizzle[] = {
				GL_RED,
				GL_RED,
				GL_RED,
				GL_ONE
			};
			glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
		} break;
		case PH_TEXTURE_FORMAT_RGB:  {
			glInternalFormat = GL_RGB;
			glFormat = GL_RGB;
		} break;
		case PH_TEXTURE_FORMAT_RGBA: {
			glInternalFormat = GL_RGBA;
			glFormat = GL_RGBA;
		} break;
		default: glFormat = 0;
	}
	glTexImage2D(GL_TEXTURE_2D, 0, glInternalFormat, t.width, t.height, 0, glFormat, GL_UNSIGNED_BYTE, data);
	
	glGenerateMipmap(GL_TEXTURE_2D);
	

	glBindTexture(GL_TEXTURE_2D, 0);
	
	if (t.type == PH_TEXTURE_TYPE_DYNAMIC) {
		if (reallocData) {
			t.dyn.data = (uint8_t*)memalloc(t.width * t.height * format);
			memcpy(t.dyn.data, data, t.width * t.height * format);
		}
		else {
			t.dyn.data = data;
		}
	}

	else if (t.type == PH_TEXTURE_TYPE_RENDER) {
		glGenFramebuffers(1, &t.ren.framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, t.ren.framebuffer);

		glGenRenderbuffers(1, &t.ren.depthTexture);
		glBindRenderbuffer(GL_RENDERBUFFER, t.ren.depthTexture);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, t.width, t.height);

		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, t.texture, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, t.ren.depthTexture);

		GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
		glDrawBuffers(sizeof(drawBuffers) / sizeof(GLenum), drawBuffers);
		
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		// if (reallocData) {
		// 	printf("IDK!?\n");
		// }
		// else {
		// 	t.dyn.data = data;
		// }
	}

	texture_ptr ptr = (texture_ptr)memalloc(sizeof(texture_t));
	*ptr = t;
	cacheAddress(&textureCache, ptr);
	return ptr;
}
texture_ptr phNewTexture_readFile(c_string_t name, int filter, textureType_e type) {
	texture_t t;
	uint8_t *data;
	int32_t  format;
	int32_t  channels;
	string_t filePath;

	t.filter = filter;
	t.type   = type;

	filePath = newstring(PH_ASSETS_TEXTURE_FOLDER);
	stringcat_buf(&filePath, name);
	// stringcat_buf(&filePath, ".png");
	data = stbi_load(filePath.buf, &t.width, &t.height, &channels, 0);

	if (data == NULL) return NULL;

	texture_ptr ptr;

	if (type == PH_TEXTURE_TYPE_STATIC) {
		ptr = phNewTexture(data, t.width, t.height, channels, filter, type);
		memfree(data);
	}
	else if (type == PH_TEXTURE_TYPE_DYNAMIC) {
		ptr = phNewTexture(data, t.width, t.height, channels, filter, type, false);
	}

	return ptr;
}

texture_ptr phNewTexture_render(int width, int height, int filter) {
	texture_ptr ptr = phNewTexture(NULL, width, height, PH_TEXTURE_FORMAT_RGB, filter, PH_TEXTURE_TYPE_RENDER);

	return ptr;
}

int phReloadTexture_render(texture_ptr* t, int width, int height, int filter) {
	texture_ptr newTexture = phNewTexture_render(width, height, filter);
	if (newTexture == NULL) return 1;

	for (size_t i = 0; i < PH_TEXTURE_GL_STACK_SLOTS_LEN; i++) {
		swap(&textureStack[i], *t, newTexture);
	}

	phDelTexture(*t);
	(*t) = newTexture;
	
	for (size_t i = 0; i < PH_TEXTURE_GL_STACK_SLOTS_LEN; i++) {
		if (phGLPeekTexture(i) == newTexture) {
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, t == NULL ? 0 : (*t)->texture);
			glActiveTexture(0);
		}
	}
	return 0;
}

void phDelTexture(texture_ptr t) {
	int found = removeAddress(&textureCache, t);
	if (!found) return;
	glDeleteTextures(1, &t->texture);

	if (t->type == PH_TEXTURE_TYPE_DYNAMIC) {
		memfree(t->dyn.data);
	}
	memfree(t);
}
