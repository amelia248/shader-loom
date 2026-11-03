#ifdef INTELLISENSE
	#define GLAPI
	#define APIENTRY
	#define GL_GLEXT_PROTOTYPES
	#include <glad/glcorearb.h>
#endif
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stddef.h>
#include <stdio.h>

#include <freetype/freetype.h>

#include <memory.h>
#include <maths.h>

#define PHOTON_FEATURE__OPENGL_STACK_OPERATIONS
#include "../photon.h"

void sleep(int milliseconds);
bool nanosleep(long long ns);
void updateInput();

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void resizeCallback(GLFWwindow* window, int width, int height);
void mousePositionCallback(GLFWwindow* window, double x, double y);
void mouseWheelCallback(GLFWwindow* window, double x, double y);

window_t photonWindow;

// matrix_t model;
// matrix_t view;
// matrix_t projection;
// matrix_t m4x4identity;

extern addressCache_t stringCache, matrixCache, shaderCache, meshCache, textureCache, fontCache;
extern addressStack_t meshStack, shaderStack, textureStack[PH_TEXTURE_GL_STACK_SLOTS_LEN], fontStack, scissorStack;
void generatePrimitiveAssets();
void generateRenderPassData();

error_t phInitWindow(c_string_t title, int width, int height, int fps) {

	stringCache  = newCache(PH_STRING_CACHE_LEN);
	matrixCache  = newCache(PH_MATRIX_CACHE_LEN);
	shaderCache  = newCache(PH_SHADER_CACHE_LEN);  shaderStack  = newStack(PH_SHADER_GL_STACK_LEN);
	meshCache    = newCache(PH_MESH_CACHE_LEN);    meshStack    = newStack(PH_MESH_GL_STACK_LEN);
	
	textureCache = newCache(PH_TEXTURE_CACHE_LEN);
	for (size_t i = 0; i < PH_TEXTURE_GL_STACK_SLOTS_LEN; textureStack[i++] = newStack(PH_TEXTURE_GL_STACK_LEN));
	
	fontCache    = newCache(PH_FONT_CACHE_LEN);    fontStack    = newStack(PH_FONT_GL_STACK_LEN);
	
	scissorStack = newStack(PH_SCISSOR_GL_STACK_LEN);

	photonWindow = (window_t){0};

	photonWindow.width     = width;
	photonWindow.height    = height;
	photonWindow.targetFPS = fps;
	photonWindow.title     = newstring(title);

	glfwInit();
	// https://docs.rs/glfw/latest/glfw/enum.WindowHint.html
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// glfwWindowHint(GLFW_DECORATED, false);

	photonWindow.glfwPtr = glfwCreateWindow(photonWindow.width, photonWindow.height, photonWindow.title.buf, NULL, NULL);
	if (photonWindow.glfwPtr == NULL) {
		printf("Failed to create GLFW window\n");
		phCloseWindow();
		return PH_ERR_FAILURE;
	}
	glfwMakeContextCurrent(photonWindow.glfwPtr);

	/* set window position to be in the centre*/ {
		GLFWmonitor *monitorptr = glfwGetPrimaryMonitor();
		if (monitorptr != NULL) {
			int x,y;
			glfwGetMonitorWorkarea(monitorptr, NULL, NULL, &x, &y);
			x = (x - photonWindow.width)  / 2;
			y = (y - photonWindow.height) / 2;
			glfwSetWindowPos(photonWindow.glfwPtr, x, y);
		}
	}

	glfwSetKeyCallback(photonWindow.glfwPtr,             keyCallback);
	glfwSetMouseButtonCallback(photonWindow.glfwPtr,     mouseButtonCallback);
	glfwSetFramebufferSizeCallback(photonWindow.glfwPtr, resizeCallback);

	glfwSetCursorPosCallback(photonWindow.glfwPtr,       mousePositionCallback);
	glfwSetScrollCallback(photonWindow.glfwPtr,          mouseWheelCallback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		printf("Failed to initialize GLAD\n");
		phCloseWindow();
		return PH_ERR_FAILURE;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0,0,photonWindow.width,photonWindow.height);
	
	// glCullFace(GL_BACK);
	// glEnable(GL_CULL_FACE);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	photonWindow.primitives.matrices.m4x4identity = phNewMatrix(4,4,
		1.,0.,0.,0.,
		0.,1.,0.,0.,
		0.,0.,1.,0.,
		0.,0.,0.,1.
	);
	photonWindow.primitives.matrices.model      = phCopyMatrix(photonWindow.primitives.matrices.m4x4identity);
	photonWindow.primitives.matrices.view       = phCopyMatrix(photonWindow.primitives.matrices.m4x4identity);
	photonWindow.primitives.matrices.projection = phCopyMatrix(photonWindow.primitives.matrices.m4x4identity);
	glEnable(GL_SCISSOR_TEST);
	
	glfwSwapInterval(0);

	//onResize(&windowPtr, windowPtr.width, windowPtr.height);
	resizeCallback(photonWindow.glfwPtr, photonWindow.width, photonWindow.height);
	photonWindow.onResize = NULL;
	// int w,h;
	// glfwGetFramebufferSize(windowPtr.glfwPtr, &w, &h);
	// printf("w %d  h %d\n", w,h);

	FT_Error ftErr = FT_Init_FreeType(&photonWindow.freetypeLibrary);
	if (ftErr) {
		printf("Failed to initialize Freetype\n");
		phCloseWindow();
		return PH_ERR_FAILURE;
	}
	phguiInit();

	generatePrimitiveAssets();
	generateRenderPassData();

	// int max_v_uniforms, max_f_uniforms;
	// glGetIntegerv(GL_MAX_VERTEX_UNIFORM_BLOCKS, &max_v_uniforms);
	// glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS, &max_f_uniforms);
	// printf("max v: %d | f: %d\n", max_v_uniforms, max_f_uniforms);

	return PH_ERR_SUCCESS;
}

#pragma region PRIMITIVES
void generatePrimitiveAssets() {
	vertexAttribute_t quadMeshVertexAttributes[] = {
		{.type = PH_TYPE_FLOAT, .length = 3}, // x, y, z
		{.type = PH_TYPE_FLOAT, .length = 2}, // u, v
	};
	float quadVertices[] = {
	//x,y,z, u,v
		0,0,1, 0,1, /* 0-----1   01---11 */
		1,0,1, 1,1, /* |   / |   |   / | */
		0,1,1, 0,0, /* | /   |   | /   | */
		1,1,1, 1,0, /* 2-----3   00---10 */
	};
	uint16_t quadIndices[] = {0,1,2, 3,2,1};
	photonWindow.primitives.quadMesh = phNewMesh(
		quadMeshVertexAttributes, sizeof(quadMeshVertexAttributes) / sizeof(vertexAttribute_t),
		quadVertices, sizeof(quadVertices),
		quadIndices, sizeof(quadIndices)
	);

	size_t texSize = 32;                                             // ┌----32----┐
	size_t texCheckerSize = 16;                                      // ┌-16-┐000000  ┐
	colour_t mergedTexData[texSize*texSize];                         // 000000ffffff 32
	memset(mergedTexData, 0xff, texSize*texSize * sizeof(colour_t)); // ffffff000000  ┘
	for (size_t i = 0; i < texSize*texSize; i++) {
		size_t x = (i%texSize) / texCheckerSize;
		size_t y = (i/texSize) / texCheckerSize;
		if ((x + y) % 2 == 1) {
			mergedTexData[i] = rgb(0x000);
		}
	}
	photonWindow.primitives.blankTex   = phNewTexture((uint8_t*)mergedTexData, 1,      1,       PH_TEXTURE_FORMAT_RGBA, GL_NEAREST, PH_TEXTURE_TYPE_STATIC);
	photonWindow.primitives.checkerTex = phNewTexture((uint8_t*)mergedTexData, texSize,texSize, PH_TEXTURE_FORMAT_RGBA, GL_NEAREST, PH_TEXTURE_TYPE_STATIC);
	photonWindow.primitives.iconsTex   = phNewTexture_readFile("vector icons.png", GL_LINEAR, PH_TEXTURE_TYPE_STATIC);

	photonWindow.primitives.internalFont = phNewFont(PH_ASSETS_PRIMITIVE_FONT, PH_ASSETS_PRIMITIVE_FONT_SIZE, PH_FONT_RENDER_MODE_LCD_RGB);
	phGenerateTextRenderer(photonWindow.primitives.internalFont);

	photonWindow.primitives.basicShader = phNewShader(PH_ASSETS_PRIMITIVE_BASIC_SHADER, PH_ASSETS_PRIMITIVE_BASIC_SHADER);
	phAddUniforms(photonWindow.primitives.basicShader,
		"model", "view", "projection",
		"uTexture", "uColour"
	);
	phGLPushShader(photonWindow.primitives.basicShader);
		phSetUniformf("uTextureA", 'tex', 0);
	phGLPopShader();

	photonWindow.primitives.textShader = phNewShader(PH_ASSETS_PRIMITIVE_TEXT_SHADER, PH_ASSETS_PRIMITIVE_TEXT_SHADER);
	phAddUniforms(photonWindow.primitives.textShader,
		"view", "projection",
		"uGlyphAtlas", "uBackgroundTexture"
	);
	phGLPushShader(photonWindow.primitives.textShader);
		phSetUniformf("uGlyphAtlas", 'tex', 1);
		phSetUniformf("uBackgroundTexture", 'tex', 2);
	phGLPopShader();

	photonWindow.primitives.rectangleShader = phNewShader(PH_ASSETS_PRIMITIVE_RECTANGLE_SHADER, PH_ASSETS_PRIMITIVE_RECTANGLE_SHADER);
	phAddUniforms(photonWindow.primitives.rectangleShader,
		"model", "view", "projection",
		"uTexture", "uColour", "uBorderColour", "uBurnColour",
		"uSize", "uCurvature", "uBorderWidth",
		"uUVRectangle"
	);
	phGLPushShader(photonWindow.primitives.rectangleShader);
		phSetUniformf("uTextureA", 'tex', 0);
	phGLPopShader();
	
}
#pragma endregion

void generateRenderPassData() {
	photonWindow.renderPasses.firstPass = phNewTexture_render(photonWindow.width, photonWindow.height, GL_NEAREST);
	
}
// also updates the window, ending the last frame & begining the next
bool phWindowShouldClose() {
	// end the current frame

	extern dvec2_t mouseWheelDelta;
	extern dvec2_t mousePrevPosition;
	extern size_t  elementMetadataCacheLen;
	extern size_t  genIDIndex;
	mouseWheelDelta = (dvec2_t){.x=0,.y=0};
	mousePrevPosition = phGetMousePosition();

	updateInput();
	glfwPollEvents();

	double frameSleepTime, frameEndTime;

	photonWindow.frameCounter++;
	photonWindow.time = glfwGetTime();

	frameEndTime   = glfwGetTime();
	frameSleepTime = (1./((double)photonWindow.targetFPS)) - (frameEndTime-photonWindow.frameStartTime);
	if (frameSleepTime > 0) {
		nanosleep(frameSleepTime*1'000'000.0);
	}

	// begin the next frame
	frameEndTime = glfwGetTime();
	photonWindow.deltaTime      = frameEndTime - photonWindow.frameStartTime;
	// printf("hovering: %f\n", photonWindow.deltaTime);
	photonWindow.frameStartTime = frameEndTime;

	// elementMetadataCacheLen = 0;

	// glScissor(0,0,photonWindow.width, photonWindow.height);
	genIDIndex = 0;
	
	return glfwWindowShouldClose(photonWindow.glfwPtr);
}

void phBeginDrawing() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
}

void phEndDrawing() {
	glfwSwapBuffers(photonWindow.glfwPtr);
}

static inline void delPtrAsShader(void* ptr) {  }

void phCloseWindow() {
	phguiClose();

	if (photonWindow.freetypeLibrary != NULL) FT_Done_FreeType(photonWindow.freetypeLibrary);
	glfwTerminate();
	
	foreachAddress(shaderCache, i) {
		phDelShader((shader_ptr)shaderCache.addresses[i]);
	}
	foreachAddress(meshCache, i) {
		phDelMesh((mesh_ptr)meshCache.addresses[i]);
	}
	foreachAddress(textureCache, i) {
		phDelTexture((texture_ptr)textureCache.addresses[i]);
	}
	foreachAddress(fontCache, i) {
		phDelFont((font_ptr)fontCache.addresses[i]);
	}

	clearCache(&matrixCache); delCache(&matrixCache);
	clearCache(&stringCache); delCache(&stringCache);
	delCache(&shaderCache);
	delCache(&meshCache);
	delCache(&textureCache);
	delCache(&fontCache);

	for (size_t i = 0; i < PH_TEXTURE_GL_STACK_SLOTS_LEN; i++) {
		delStack(&textureStack[i]);
	}
	
	delStack(&meshStack);
	delStack(&shaderStack);
	delStack(&fontStack);
	delStack(&scissorStack);

	extern elementMetadata_t elementMetadataCache[PH_MAX_ELEMENT_METADATA_ENTRIES];
	extern size_t            elementMetadataCacheLen;
	for (int i = 0; i < elementMetadataCacheLen; i++) {
		delCache(&elementMetadataCache[i].addresses);
	}

}

void phSetClearColour(colour_t c) {
	glClearColor(
		(float)c.r / 255.0,
		(float)c.g / 255.0,
		(float)c.b / 255.0,
		(float)c.a / 255.0
	);
}
inline void      phSetOnResizeFunction(void (*onResize)(int, int)) { photonWindow.onResize = onResize; }
inline int       phGetWinWidth()  { return photonWindow.width; }
inline int       phGetWinHeight() { return photonWindow.height; }
inline double    phGetDeltaTime() { return photonWindow.deltaTime; }
inline double    phGetTime()      { return glfwGetTime(); }
inline window_t* phGetWindow()    { return &photonWindow; }
inline int       phGetFrame()     { return photonWindow.frameCounter; }

// FT_Library getFreetypeLibrary() { return photonWindow.freetypeLibrary; }
