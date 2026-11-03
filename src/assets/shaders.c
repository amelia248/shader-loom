#ifdef INTELLISENSE
	#define GLAPI
	#define APIENTRY
	#define GL_GLEXT_PROTOTYPES
	#include <glad/glcorearb.h>
#endif
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>

#include <memory.h>
#include <maths.h>

#include "../photon.h"

addressCache_t shaderCache;
addressStack_t shaderStack;


void phAddUniform(shader_ptr s, c_string_t name) {
	s->uniforms[s->uniformsLen].name = newstring(name);
	s->uniforms[s->uniformsLen].loc  = glGetUniformLocation(s->program, name);

	// glGetProgramiv(s->program, GL_ACTIVE_UNIFORMS, NULL);
	// // glGetActiveUniform

	// if (s->uniforms[s->uniformsLen].loc < 0) {
	// 	printf("could not find uniform \"%s\" in shader \"%s\". uniform kept in list\n", name, s->fragmentName.buf);
	// }
	s->uniformsLen++;
}

void phAddNUniforms(shader_ptr s, size_t n, ...) {
	va_list args;
	
	va_start(args, n);
	for (size_t i = 0; i < n; i++) {
		phAddUniform(s, va_arg(args, c_string_t));
	}
	va_end(args);
}
void phAddNUniforms_strings(shader_ptr s, size_t n, string_t* names) {
	for (size_t i = 0; i < n; i++) {
		phAddUniform(s, names[i].buf);
	}
}


void phGLPushShader(shader_ptr s) {
	push(&shaderStack, s);
	glUseProgram(s->program);
}
shader_ptr phGLPopShader() {
	shader_ptr ret = pop(&shaderStack);
	shader_ptr s = peek(&shaderStack);
	glUseProgram(s == NULL ? 0 : s->program);
	return ret;
}
shader_ptr phGLPeekShader() { return peek(&shaderStack); }

#define REPEAT_1(_x) _x
#define REPEAT_2(_x) _x, _x
#define REPEAT_3(_x) _x, _x, _x
#define REPEAT_4(_x) _x, _x, _x, _x

void phSetUniformf(c_string_t name, uint32_t fmt, ...) {
	va_list args;
	shader_ptr s = phGLPeekShader();
	int loc = -1;
	size_t nameLen = strlen(name);
	size_t i = 0;

	double dvector[4]; int ivector[4];
	// size_t i = 0;
	while (i < s->uniformsLen && !(s->uniforms[i].name.len == nameLen &&
				strncmp(name, s->uniforms[i].name.buf, nameLen) == 0)) i++;
	if (i >= s->uniformsLen) return;
	loc = s->uniforms[i].loc;
	// do {
	// 	i++;
	// 	loc = s->uniforms[i].loc;
	// } while (strcmp(name, s->uniforms[i].name.buf) != 0);
	
	if (loc < 0) return;
	
	va_start(args, fmt);
	switch (fmt)
	{
	case '1f': case 'f': {
		for (size_t i=0; i<1; i++) dvector[i] = va_arg(args, double);
		glUniform1f(loc, dvector[0]);
		// GLenum err = glGetError();
		// if (err != GL_NO_ERROR) printf("GL error: 0x%x\n", err);
	} break;
	case '2f': case 'ff': case 'v2': {
		for (size_t i=0; i<2; i++) dvector[i] = va_arg(args, double);
		glUniform2f(loc, dvector[0], dvector[1]);
	} break;
	case '3f': case 'fff': case 'v3': {
		for (size_t i=0; i<3; i++) dvector[i] = va_arg(args, double);
		glUniform3f(loc, dvector[0], dvector[1], dvector[2]);
	} break;
	case '4f': case 'ffff': case 'v4': {
		for (size_t i=0; i<4; i++) dvector[i] = va_arg(args, double);
		glUniform4f(loc, dvector[0], dvector[1], dvector[2], dvector[3]);
	} break;

	case '1i': case 'i': {
	for (size_t i=0; i<1; i++) ivector[i] = va_arg(args, int);
		glUniform1i(loc, ivector[0]);
		// GLenum err = glGetError();
		// if (err != GL_NO_ERROR) printf("GL error: 0x%x\n", err);
	} break;
	case '2i': case 'ii': case 'iv2': {
		for (size_t i=0; i<2; i++) ivector[i] = va_arg(args, int);
		glUniform2i(loc, ivector[0], ivector[1]);
	} break;
	case '3i': case 'iii': case 'iv3': {
		for (size_t i=0; i<3; i++) ivector[i] = va_arg(args, int);
		glUniform3i(loc, ivector[0], ivector[1], ivector[2]);
	} break;
	case '4i': case 'iiii': case 'iv4': {
		for (size_t i=0; i<4; i++) ivector[i] = va_arg(args, int);
		glUniform4i(loc, ivector[0], ivector[1], ivector[2], ivector[3]);
	} break;

	
	case 'c4': case 'col4': case 'rgba': {
		colour_t col = va_arg(args, colour_t);
		glUniform4f(loc,
			(float)col.r / 255.0,
			(float)col.g / 255.0,
			(float)col.b / 255.0,
			(float)col.a / 255.0
		);
	} break;
	case 't': case 'tex': {
		int slot = va_arg(args, int);
		glUniform1i(loc, slot);
	} break;

	case 'm4': case 'm4x4':  glUniformMatrix4fv(loc, 1, 0, va_arg(args, matrix_t).data); break;
	// case 'm4t': case '4x4t': glUniformMatrix4fv(loc, 1, 1,  va_arg(args, matrix_t).data); break;
	
	// default: {
	// 	if (fmt>>24 == 't' || fmt>>16 == 't' || fmt>>8 == 't') {
	// 		// texture_ptr tex = va_arg(args, texture_ptr);
	// 		int activeTexNum = 0;
	// 		int pow = 1;
	// 		while ((fmt&0xff) != 't') {
	// 			activeTexNum += ((fmt & 0xff) - 0x30) * pow;
	// 			pow *= 10;
	// 			__asm__ ("rorl %%cl, %0" : "+r" (fmt) : "c" (8)); // 't123' -> '123t'
	// 		}
	// 		// glActiveTexture(GL_TEXTURE0 + activeTexNum);
	// 		// glBindTexture(GL_TEXTURE_2D, tex->texture);
	// 		glUniform1i(loc, activeTexNum);
	// 	}
	// } break;
	}
	va_end(args);
	
}

int compileShaderContent(string_t data, uint32_t shaderType, uint32_t *shader);
int checkCompileErrors(uint32_t shader, uint32_t shaderType);

shader_ptr phNewShaderFromPath(string_t vsPath, string_t fsPath) {
	int err;
	uint32_t vertexShader;
	uint32_t fragmentShader;

	string_t shaderData;
	shader_t s = {0};

	s.vertexName   = vsPath;
	s.fragmentName = fsPath;

	s.uniformsLen = 0;
	for (size_t i = 0; i < PH_MAX_SHADER_UNIFORMS_COUNT; i++)
	{
		s.uniforms[i].loc = 0;
		s.uniforms[i].name = (string_t){.len=0, .buf=NULL};
	}
	
	shaderData = newstring_readFile(vsPath.buf); err = shaderData.buf == NULL;
	retryVertexCompilation:
	if (err != 0) shaderData = newstring(PH_DEFAULT_VERTEX_SHADER_DATA);
	
	err = compileShaderContent(shaderData, GL_VERTEX_SHADER, &vertexShader);
	
	delstring(shaderData);
	if (err != 0) goto retryVertexCompilation;

	shaderData = newstring_readFile(fsPath.buf); err = shaderData.buf == NULL;
	if (strcmp(fsPath.buf+fsPath.len-2, "sl") == 0) {
		string_t shaderPrepend = newstring(""
			"#version 330\n"
			"in vec3 fragmentPosition;"
			"in vec2 fragmentCoord;"
			"out vec4 finalColour;"

			"uniform vec3      iResolution;           /* viewport resolution (in pixels) */"
			"uniform float     iTime;                 /* shader playback time (in seconds)"
			"uniform float     iTimeDelta;            /* render time (in seconds) */"
			"uniform float     iFrameRate;            /* shader frame rate */"
			"uniform int       iFrame;                /* shader playback frame */"
			// "uniform float     iChannelTime[4];       // channel playback time (in seconds)\n"
			// "uniform vec3      iChannelResolution[4]; // channel resolution (in pixels)\n"
			"uniform vec4      iMouse;                /* mouse pixel coords. xy: current (if MLB down), zw: click */"
			// "uniform samplerXX iChannel0;             // input channel. XX = 2D/Cube\n"
			// "uniform samplerXX iChannel1;             // input channel. XX = 2D/Cube\n"
			// "uniform samplerXX iChannel2;             // input channel. XX = 2D/Cube\n"
			// "uniform samplerXX iChannel3;             // input channel. XX = 2D/Cube\n"
			"uniform vec4      iDate;                 // (year, month, day, time in seconds) */\n"
			// "uniform float     iSampleRate;           // sound sample rate (i.e., 44100)\n"
		);

		string_t shaderAppend = newstring(
			"\nvoid main() {\n"
			"  vec2 uv = fragmentCoord;\n"
			"  uv.y = 1 - uv.y;\n"
			"  mainImage(finalColour, uv * iResolution.xy);\n"
			"}\n"
		);
		stringcat_str(&shaderPrepend, shaderData);
		delstring(shaderData);
		shaderData = shaderPrepend;
		stringcat_str(&shaderData, shaderAppend);
		// shaderData = stringcat_str(string_t *s, string_t source)
	}
	retryFragmentCompilation:
	if (err != 0) shaderData = newstring(PH_DEFAULT_FRAGMENT_SHADER_DATA);
	
	err = compileShaderContent(shaderData, GL_FRAGMENT_SHADER, &fragmentShader);
	
	delstring(shaderData);
	if (err != 0) goto retryFragmentCompilation;
	
	s.program = glCreateProgram();
	glAttachShader(s.program, vertexShader);
	glAttachShader(s.program, fragmentShader);
	glLinkProgram(s.program);
	// err = getShaderError_low(shader.Program, gl.GetProgramiv, gl.GetProgramInfoLog, gl.LINK_STATUS)
	
	err = checkCompileErrors(s.program, GL_ATTACHED_SHADERS);
	if (err != 0) {
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		glDeleteProgram(s.program);
		return NULL;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	shader_ptr ptr = (shader_ptr)memalloc(sizeof(shader_t));
	*ptr = s;
	cacheAddress(&shaderCache, ptr);
	return ptr;
}

shader_ptr phNewShader(c_string_t vsName, c_string_t fsName) {
	int err;
	uint32_t vertexShader;
	uint32_t fragmentShader;

	string_t filePath;
	string_t shaderData;
	shader_t s = {0};

	s.vertexName   = newstring(vsName);
	s.fragmentName = newstring(fsName);

	s.uniformsLen = 0;
	for (size_t i = 0; i < PH_MAX_SHADER_UNIFORMS_COUNT; i++)
	{
		s.uniforms[i].loc = 0;
		s.uniforms[i].name = (string_t){.len=0, .buf=NULL};
	}
	

	filePath = newstring(PH_ASSETS_SHADER_FOLDER);
	stringcat_str(&filePath, s.vertexName);
	stringcat_buf(&filePath, ".vs");
	
	shaderData = newstring_readFile(filePath.buf); err = shaderData.buf == NULL;
	retryVertexCompilation:
	if (err != 0) shaderData = newstring(PH_DEFAULT_VERTEX_SHADER_DATA);
	
	err = compileShaderContent(shaderData, GL_VERTEX_SHADER, &vertexShader);
	
	delstring(shaderData);
	if (err != 0) goto retryVertexCompilation;
	delstring(filePath);

	filePath = newstring(PH_ASSETS_SHADER_FOLDER);
	stringcat_str(&filePath, s.fragmentName);
	stringcat_buf(&filePath, ".fs");

	shaderData = newstring_readFile(filePath.buf); err = shaderData.buf == NULL;
	retryFragmentCompilation:
	if (err != 0) shaderData = newstring(PH_DEFAULT_FRAGMENT_SHADER_DATA);
	
	err = compileShaderContent(shaderData, GL_FRAGMENT_SHADER, &fragmentShader);
	
	delstring(shaderData);
	if (err != 0) goto retryFragmentCompilation;
	delstring(filePath);
	
	s.program = glCreateProgram();
	glAttachShader(s.program, vertexShader);
	glAttachShader(s.program, fragmentShader);
	glLinkProgram(s.program);
	// err = getShaderError_low(shader.Program, gl.GetProgramiv, gl.GetProgramInfoLog, gl.LINK_STATUS)
	
	err = checkCompileErrors(s.program, GL_ATTACHED_SHADERS);
	if (err != 0) {
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		glDeleteProgram(s.program);
		return NULL;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	shader_ptr ptr = (shader_ptr)memalloc(sizeof(shader_t));
	*ptr = s;
	cacheAddress(&shaderCache, ptr);
	return ptr;
}

void phDelShader(shader_ptr s) {
	int found = removeAddress(&shaderCache, s);
	if (!found) return;
	glDeleteProgram(s->program);

	delstring(s->vertexName);
	delstring(s->fragmentName);
	for (size_t i = 0; i < s->uniformsLen; i++) {
		delstring(s->uniforms[i].name);
	}

	memfree(s);
}

int phReloadShader(shader_ptr* s) {
	shader_ptr newShader = phNewShader((*s)->vertexName.buf, (*s)->fragmentName.buf);
	if (newShader == NULL) return 1;

	string_t names[(*s)->uniformsLen];
	for (size_t i = 0; i < (*s)->uniformsLen; i++) {
		names[i] = (*s)->uniforms[i].name;
	}
	phAddNUniforms_strings(newShader, (*s)->uniformsLen, names);

	// replace any entries in the stack with the new shader address
	swap(&shaderStack, *s, newShader);

	phDelShader(*s);
	(*s) = newShader;

	if (phGLPeekShader() == *s) {
		glUseProgram((*s)->program);
	}
	return 0;
}


int compileShaderContent(string_t data, uint32_t shaderType, uint32_t *shader) {
	int err;
	(*shader) = glCreateShader(shaderType);
	c_string_t dbuf = data.buf;
	const int   dlen = data.len;
	glShaderSource(*shader, 1, &dbuf, NULL);

	glCompileShader(*shader);

	err = checkCompileErrors(*shader, shaderType);
	if (err != 0) {
		glDeleteShader(*shader);
		return -1;
	}

	return 0;
}

int checkCompileErrors(uint32_t shader, uint32_t shaderType) {
	int success, logLen;
	char log[1024];

	if (shaderType == GL_ATTACHED_SHADERS) {
		glGetProgramiv(shader, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &logLen);
			glGetProgramInfoLog(shader, logLen, NULL, log);
			printf("SHADER COMPILE ERROR: %*s", logLen, log);
			return 1;
		}
	}
	else {
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
			glGetShaderInfoLog(shader, logLen, NULL, log);
			printf("SHADER COMPILE ERROR: %*s", logLen, log);
			return 1;
		}
	}

	return 0;
}

// use a shader arena for storing
// push & pop from an active-shader-stack