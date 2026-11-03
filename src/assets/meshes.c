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

addressCache_t meshCache;
addressStack_t meshStack;

void phGLPushMesh(mesh_ptr m) {
	push(&meshStack, m);
	glBindVertexArray(m->VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->EBO);
}
mesh_ptr phGLPopMesh() {
	mesh_ptr ret = pop(&meshStack);
	mesh_ptr m = peek(&meshStack);
	glBindVertexArray(m == NULL ? 0 : m->VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m == NULL ? 0 : m->EBO);
	return ret;
}
mesh_ptr phGLPeekMesh() {
	return peek(&meshStack);
}

mesh_ptr phNewMesh(vertexAttribute_t* attributes, size_t attributesLen, const float *vertices, size_t vsize, const uint16_t *indices, size_t isize) {
	mesh_t m = {0};

	m.vertices = (float*)memalloc(vsize);
	m.verticesLen = vsize / sizeof(float);
	memcpy(m.vertices, vertices, vsize);

	m.indices = (uint16_t*)memalloc(isize);
	m.indicesLen = isize / sizeof(uint16_t);
	memcpy(m.indices, indices, isize);

	glGenVertexArrays(1, &m.VAO);
	glGenBuffers(1, &m.VBO);
	glGenBuffers(1, &m.EBO);

	glBindVertexArray(m.VAO);

	glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
	glBufferData(GL_ARRAY_BUFFER, vsize, m.vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, isize, m.indices, GL_STATIC_DRAW);

	
	size_t stride = 0;
	for (size_t i = 0; i < attributesLen; i++) {
		stride += attributes[i].length * sizeofType_t(attributes[i].type);
	}
	size_t offset = 0;
	for (size_t i = 0; i < attributesLen; i++) {
		GLenum glType;
		switch (attributes[i].type) {
			case PH_TYPE_FLOAT: glType = GL_FLOAT; break;
			case PH_TYPE_INT:   glType = GL_INT; break;
		}
		glVertexAttribPointer(i, attributes[i].length, glType, GL_FALSE, stride, (void*)offset);
		glEnableVertexAttribArray(i);
		offset += attributes[i].length * sizeofType_t(attributes[i].type);
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	mesh_ptr ptr = (mesh_ptr)memalloc(sizeof(mesh_t));
	*ptr = m;
	cacheAddress(&meshCache, ptr);
	return ptr;
}

mesh_ptr phNewBatchedMesh(vertexAttribute_t* attributes, size_t attributesLen, const uint16_t* indicesPattern, size_t isize, size_t maxInstances) {
	mesh_t m = {0};
	
	const size_t indicesLen = isize/sizeof(uint16_t);
	int uniqueIndicesValues[indicesLen];
	int uniqueIndicesCount = 0;
	for (size_t i = 0; i < indicesLen; i++) {
		bool inList = false;
		for (size_t j = 0; j < uniqueIndicesCount; j++) {
			if (indicesPattern[i] == uniqueIndicesValues[j]) {
				inList = true;
			}
		}
		if (!inList) {
			uniqueIndicesValues[uniqueIndicesCount++] = indicesPattern[i];
		}
	}
	size_t vertexSize = 0;

	for (size_t i = 0; i < attributesLen; i++) {
		vertexSize += attributes[i].length;
	}
	

	const size_t verticesLen = vertexSize * maxInstances * uniqueIndicesCount;

	m.vertices = (float*)memalloc(verticesLen * sizeof(float));
	m.verticesLen = verticesLen;

	m.indices = (uint16_t*)memalloc(isize * maxInstances);
	m.indicesLen = indicesLen;
	memcpy(m.indices, indicesPattern, isize);
	for (size_t i = 1; i < maxInstances; i++) {
		for (size_t j = 0; j < indicesLen; j++) {
			m.indices[i*indicesLen + j] = m.indices[(i-1)*indicesLen + j] + uniqueIndicesCount;
		}
	}
	
	glGenVertexArrays(1, &m.VAO);
	glGenBuffers(1, &m.VBO);
	glGenBuffers(1, &m.EBO);

	glBindVertexArray(m.VAO);

	glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
	glBufferData(GL_ARRAY_BUFFER, verticesLen * sizeof(float), m.vertices, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, isize * maxInstances, m.indices, GL_STATIC_DRAW);

	size_t stride = 0;
	for (size_t i = 0; i < attributesLen; i++) {
		stride += attributes[i].length * sizeofType_t(attributes[i].type);
	}
	size_t offset = 0;
	for (size_t i = 0; i < attributesLen; i++) {
		GLenum glType;
		switch (attributes[i].type) {
			case PH_TYPE_FLOAT: glType = GL_FLOAT; break;
			case PH_TYPE_INT:   glType = GL_INT; break;
		}
		glVertexAttribPointer(i, attributes[i].length, glType, GL_FALSE, stride, (void*)offset);
		glEnableVertexAttribArray(i);
		offset += attributes[i].length * sizeofType_t(attributes[i].type);
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	mesh_ptr ptr = (mesh_ptr)memalloc(sizeof(mesh_t));
	*ptr = m;
	cacheAddress(&meshCache, ptr);
	return ptr;
}

void phDelMesh(mesh_ptr m) {
	int found = removeAddress(&meshCache, m);
	if (!found) return;
	glDeleteVertexArrays(1, &m->VAO);
	glDeleteBuffers(1, &m->VBO);
	glDeleteBuffers(1, &m->EBO);

	memfree(m->vertices);
	memfree(m->indices);

	memfree(m);
}