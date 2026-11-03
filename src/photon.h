#ifndef PHOTON_SRC_MAIN_H
#define PHOTON_SRC_MAIN_H

#pragma region TO DO
// cache .ttf textures as bitmaps/png's
#pragma endregion

#include <stddef.h>

#include <freetype/freetype.h>
#include <clay/clay.h>

#include <memory.h>
#include <errors.h>
#include <maths.h>


#pragma region MEMORY CAPS
#define PH_MATRIX_CACHE_LEN 32
#define PH_STRING_CACHE_LEN 512

#define PH_SHADER_CACHE_LEN 32
#define PH_MAX_SHADER_UNIFORMS_COUNT 32

#define PH_MESH_CACHE_LEN 16

#define PH_TEXTURE_CACHE_LEN 32
#define PH_TEXTURE_GL_STACK_SLOTS_LEN 16

#define PH_FONT_CACHE_LEN 16
#define PH_FONT_RENDERER_MAX_GLYPH_INSTANCES 512
#define PH_TEXT_RENDERER_MAX_TEXT_ELEMENTS   2048

#define PH_MAX_SCISSORS_PER_FRAME 128

#define PH_SHADER_GL_STACK_LEN  32
#define PH_MESH_GL_STACK_LEN    32
#define PH_TEXTURE_GL_STACK_LEN 32
#define PH_FONT_GL_STACK_LEN    32
#define PH_SCISSOR_GL_STACK_LEN 64

#define PH_MAX_ELEMENT_METADATA_ENTRIES   1024
#define PH_ELEMENT_METADATA_MAX_ADDRESSES 128

#define PH_ASSETS_SHADER_FOLDER  "assets/shaders/"
#define PH_ASSETS_TEXTURE_FOLDER "assets/textures/"
#define PH_ASSETS_FONT_FOLDER    "assets/fonts/"

#define PH_ASSETS_PRIMITIVE_BASIC_SHADER     "330/main"
#define PH_ASSETS_PRIMITIVE_TEXT_SHADER      "330/text"
#define PH_ASSETS_PRIMITIVE_RECTANGLE_SHADER "330/rectangle"
#define PH_ASSETS_PRIMITIVE_FONT             "fira/semi-bold.ttf"
// #define PH_ASSETS_PRIMITIVE_FONT             "atkinson/regular.ttf"
#define PH_ASSETS_PRIMITIVE_FONT_SIZE        14
#pragma endregion

#pragma region SHADERS.C

typedef struct {
	uint32_t program;
	
	struct {
		int32_t  loc;
		string_t name;
	} uniforms[PH_MAX_SHADER_UNIFORMS_COUNT];
	uint8_t uniformsLen;

	// metadata:
	string_t vertexName;
	string_t fragmentName;
} shader_t;

// typedef size_t shader_i;
typedef shader_t* shader_ptr;

shader_ptr phNewShader(c_string_t vsName, c_string_t fsName);
shader_ptr phNewShaderFromPath(string_t vsPath, string_t fsPath);
void       phDelShader(shader_ptr s);
int        phReloadShader(shader_ptr* s);

void phAddUniform(shader_ptr s, c_string_t name);
void phAddNUniforms(shader_ptr s, size_t count, ...);
void phAddNUniforms_strings(shader_ptr s, size_t n, string_t* names);
void phSetUniformf(c_string_t name, uint32_t fmt, ...);

#define phAddUniforms(s, ...) phAddNUniforms(s, (sizeof((char*[]){__VA_ARGS__})/sizeof(char*)), __VA_ARGS__)


#define PH_DEFAULT_VERTEX_SHADER_DATA \
"#version 330\n"                    \
"\n"                                 \
"in vec3 vertexPosition;\n"           \
"in vec2 textureCoord;\n"              \
"\n"                                    \
"uniform mat4 model;\n"                  \
"uniform mat4 view;\n"                    \
"uniform mat4 projection;\n"               \
"\n"                                        \
"out vec3 fragmentPosition;\n"               \
"out vec2 fragmentCoord;\n"                   \
"\n"                                           \
"void main() {\n"                               \
"\tmat4 mvp = projection*view*model;\n"          \
"\n"                                              \
"\tgl_Position = mvp*vec4(vertexPosition,1);\n"    \
"\tfragmentPosition = gl_Position.xyz;\n"           \
"\tfragmentCoord = textureCoord;\n"                  \
"}\n"

#define PH_DEFAULT_FRAGMENT_SHADER_DATA                      \
"#version 330\n"                                           \
"\n"                                                        \
"in vec3 fragmentPosition;\n"                                \
"in vec2 fragmentCoord;\n"                                    \
"\n"                                                           \
"out vec4 finalColor;\n"                                        \
"\n"                                                             \
"void main() {\n"                                                 \
"\tvec2 uv = fragmentCoord;\n"                                     \
"\tuv.x += (mod(uv.y*5., 1.) < .5 ? 0. : .25);\n"                   \
"\tfinalColor = vec4(1,0,.5, 1) * (mod(uv.x*5., 1.)<.5 ? 0. : 1.);\n"\
"}\n"

#pragma endregion

#pragma region MESHES.C

typedef struct {
	uint32_t VBO;
	uint32_t VAO;
	uint32_t EBO;

	float    *vertices;
	uint16_t *indices;

	uint16_t verticesLen;
	uint16_t indicesLen;
} mesh_t;

typedef struct {
	type_t type;
	int length;
} vertexAttribute_t;

typedef mesh_t* mesh_ptr;

// mesh_ptr phNewMesh(const float *vertices, size_t vsize, const uint16_t *indices, size_t isize);
mesh_ptr phNewMesh(vertexAttribute_t* attributes, size_t attributesLen, const float *vertices, size_t vsize, const uint16_t *indices, size_t isize);
mesh_ptr phNewBatchedMesh(vertexAttribute_t* attributes, size_t attributesLen, const uint16_t* indicesPattern, size_t isize, size_t maxInstances);
void     phDelMesh(mesh_ptr m);

#pragma endregion

#pragma region TEXTURES.C

typedef enum {
	PH_TEXTURE_TYPE_STATIC  = 0,
	PH_TEXTURE_TYPE_DYNAMIC = 1,
	PH_TEXTURE_TYPE_RENDER  = 2,
} textureType_e;

typedef enum {
	PH_TEXTURE_FORMAT_R    = 1,
	PH_TEXTURE_FORMAT_RGB  = 3,
	PH_TEXTURE_FORMAT_RGBA = 4,
} textureFormat_e;

typedef struct {
	uint8_t *data;
} dynamicTexMetadata_t;

typedef struct {
	// uint8_t *data;
	uint32_t framebuffer;
	uint32_t depthTexture;
} renderTexMetadata_t;

typedef struct {
	uint32_t texture;
	int32_t index, width, height, filter;
	textureType_e type; // determines what metadata to use
	union {
		dynamicTexMetadata_t dyn;
		renderTexMetadata_t ren;
	};
} texture_t;

typedef texture_t* texture_ptr;

texture_ptr phNewTexture(uint8_t* data, int width, int height, textureFormat_e format, int filter, textureType_e type, ...);
texture_ptr phNewTexture_readFile(c_string_t path, int filter, textureType_e type);
texture_ptr phNewTexture_render(int width, int height, int filter);
int         phReloadTexture_render(texture_ptr* t, int width, int height, int filter);
void        phDelTexture(texture_ptr t);

// mesh_ptr phNewMesh(const float *vertices, size_t vsize, const uint16_t *indices, size_t isize);
// void     phDelMesh(mesh_ptr m);

#pragma endregion

#pragma region FONTS.C

typedef struct {
	float x,y,z;
	float r,g,b,a;
	float u,v;
} fontMeshVertex_t;

typedef struct {
	fontMeshVertex_t* vertices;

	mesh_ptr batchedMesh;
} textRenderer_t;

typedef struct {
	size_t value;
	irec32_t rec;
	ivec2_t offset;
	ivec2_t advance;
} glyph_t; 

typedef struct {
	size_t id;
	int size;
	int ascender, descender;
	texture_ptr texture;
	glyph_t* glyphs;
	size_t glyphsCount;
	textRenderer_t* renderer;
} font_t;

typedef font_t* font_ptr;

typedef enum {
	PH_FONT_RENDER_MODE_NORMAL  = 0,
	PH_FONT_RENDER_MODE_LCD_RGB = 1,
	// PH_FONT_RENDER_MODE_LCD_BGR = 2,
	PH_FONT_RENDER_MODE_SDF     = 3,
} fontRenderMode_e;

textRenderer_t newTextRenderer(font_ptr f);

font_ptr phNewFont(c_string_t name, uint32_t size, ...);
void phGenerateTextRenderer(font_ptr f);
void phDelFont(font_ptr f);

ivec2_t phMeasureText(font_ptr font, c_string_t text, size_t length, uint16_t size, int16_t hSpacing, int16_t vSpacing);

#pragma endregion

#pragma region SCISSORS.C

typedef struct {
	uint16_t x,y,w,h;
} scissor_t;

void phGLSetScissor(scissor_t s);

#pragma endregion

#pragma region OPTIONAL
#ifdef PHOTON_FEATURE__OPENGL_STACK_OPERATIONS

void       phGLPushShader(shader_ptr s);
shader_ptr phGLPopShader();
shader_ptr phGLPeekShader();

void     phGLPushMesh(mesh_ptr m);
mesh_ptr phGLPopMesh();
mesh_ptr phGLPeekMesh();

void        phGLPushTexture(int glSlot, texture_ptr t);
texture_ptr phGLPopTexture(int glSlot);
texture_ptr phGLPeekTexture(int glSlot);

void     phGLPushFont(font_ptr f);
font_ptr phGLPopFont();
font_ptr phGLPeekFont();

void      phGLPushScissor(scissor_t s);
scissor_t phGLPopScissor();
scissor_t phGLPeekScissor();

#endif
#pragma endregion

#pragma region INPUT.C

typedef enum {
	KeyUp       = 0b00000001,
	KeyPressed  = 0b00000010,
	KeyDown     = 0b00010100,
	KeyReleased = 0b00001000,
	KeyRepeated = 0b00010010,
} keyStateMask_t;

typedef enum {
	MouseUp       = 0b00000001,
	MousePressed  = 0b00000010,
	MouseDown     = 0b00000100,
	MouseReleased = 0b00001000,
} mouseButtonState_t;

bool phIsKey(int key, keyStateMask_t state);
bool phIsMouseButton(int button, mouseButtonState_t state);

dvec2_t phGetMousePosition();
dvec2_t phGetMouseDelta();
dvec2_t phGetMouseWheelDelta();

#pragma endregion

#pragma region CLAY_INT.C

void phguiInit();
void phguiClose();

void phguiBeginLayout();
void phguiEndLayout();

void phguiRender();

#pragma endregion

#pragma region CLAY_UTILS.C
Clay_Color phColourToClayColour(colour_t c);
colour_t   phClayColourToPhColour(Clay_Color c);

Clay_Color phLerpClayColour(Clay_Color a, Clay_Color b, double t);
#pragma endregion

#pragma region UI_BEHAVIOUR.C

typedef ivec2_t icon_t;

typedef enum {
	PH_META_ELE_BUTTON,
	PH_META_ELE_TOGGLE,
	PH_META_ELE_GRABBABLE,
	PH_META_ELE_SLIDER,
	PH_META_ELE_PAN_AND_ZOOM,
	PH_META_ELE_ICON,
	PH_META_ELE_9_SLICED_ICON,
} elementMetadataType_e;

typedef struct {
	Clay_ElementId id;
	elementMetadataType_e type;
	double timeDelta;

	addressCache_t addresses;

	union {
		struct {
			// double timeDelta;
			double hoverTime;
			bool   mouseStartedOnElement;
		} button;

		struct {
			// double timeDelta;
			double hoverTime;
			double stateTime;
			bool   mouseStartedOnElement;
			bool   state;
		} toggle;

		struct {
			bool   grabbed;
			int    delta;
			int    snapshot;
			double timer;
		} grabbable;

		struct {
			bool   grabbed;
			double value;
			double timer;
		} slider;

		struct {
			bool  grabbed;
			dvec2_t pan;
			double  zoom;
		} panAndZoom;

		struct {
			icon_t;
			double   rotation;
			colour_t colour;
		} icon;

		struct {
			icon_t;
			double   rotation;
			colour_t burn;
			// rec
		} nineSlicedIcon;
	};
} elementMetadata_t;

typedef elementMetadata_t* elementMetadata_ptr;

elementMetadata_ptr phguiGetElementMetadata(Clay_ElementId id, elementMetadata_t init, bool* existedAlready);

bool phguiButton(Clay_ElementId id, double* timer);
bool phguiToggle(Clay_ElementId id, double* timer, double* stateTimer, bool* state);
void phguiSetToggleState(Clay_ElementId id, bool state);
bool phguiGrabbable(Clay_ElementId id, double sizingDelta, double* timer, int* snapshot, int* delta, bool* grabbed);
bool phguiSlider(Clay_ElementId id, double delta, double* timer, double* value, bool* grabbed);
bool phguiPanAndZoom(Clay_ElementId containerID, Clay_ElementId movableID, dvec2_t* pan, double* zoom, bool canInteract);

void phguiIcon(icon_t icon, double rotation, colour_t c);
void phguiNineSlicedIcon(icon_t icon, irec32_t rec, colour_t burn, int border);

typedef enum __attribute__((__packed__)) {
	PHGUI_FRAME_SIZER_FIXED,   PHGUI_FRAME_SIZER_GROW,
	PHGUI_FRAME_SIZER_PERCENT, PHGUI_FRAME_SIZER_FIT,
} phguiFrameSizer_e;


#define PHGUI_SIZER_FIT(_min)

#pragma endregion

#pragma region ICONS

#define PH_ICON_BASE_FILE            ((icon_t){ 0,0})
#define PH_ICON_FILE                 ((icon_t){ 1,0})
#define PH_ICON_FILE_PLUS            ((icon_t){ 2,0})
#define PH_ICON_FILE_E_ARROW         ((icon_t){ 3,0})
#define PH_ICON_FILE_CLOCKWISE_ARROW ((icon_t){ 4,0})
#define PH_ICON_FILE_ANTICLOCK_ARROW ((icon_t){ 5,0})
#define PH_ICON_FILE_LOCATION_PIN    ((icon_t){ 6,0})
#define PH_ICON_FILE_CLOCK           ((icon_t){ 7,0})
// #define                           ((icon_t){ 8,0})
// #define                           ((icon_t){ 9,0})
// #define                           ((icon_t){10,0})
// #define                           ((icon_t){11,0})
// #define                           ((icon_t){12,0})
// #define                           ((icon_t){13,0})
// #define                           ((icon_t){14,0})
// #define                           ((icon_t){15,0})

// #define                       ((icon_t){ 0,1})
#define PH_ICON_FOLDER_CLOSED    ((icon_t){ 1,1})
#define PH_ICON_FOLDER_OPEN      ((icon_t){ 2,1})
#define PH_ICON_FOLDER_OPEN_FILE ((icon_t){ 3,1})
// #define                       ((icon_t){ 4,1})
// #define                       ((icon_t){ 5,1})
// #define                       ((icon_t){ 6,1})
// #define                       ((icon_t){ 7,1})
// #define                       ((icon_t){ 8,1})
// #define                       ((icon_t){ 9,1})
// #define                       ((icon_t){10,1})
// #define                       ((icon_t){11,1})
// #define                       ((icon_t){12,1})
// #define                       ((icon_t){13,1})
// #define                       ((icon_t){14,1})
// #define                       ((icon_t){15,1})

// #define                       ((icon_t){ 0,3})
#define PH_ICON_WINDOW_MIN       ((icon_t){ 1,3})
#define PH_ICON_WINDOW_MAX       ((icon_t){ 2,3})
#define PH_ICON_WINDOW_CLOSE     ((icon_t){ 3,3})
#define PH_ICON_CHECK_MARK       ((icon_t){ 4,3})
#define PH_ICON_ELLIPSIS         ((icon_t){ 5,3})
#define PH_ICON_MAGNIFYING_GLASS ((icon_t){ 6,3})
#define PH_ICON_ANTICLOCK_ARROW  ((icon_t){ 7,3})
#define PH_ICON_CLOCKWISE_ARROW  ((icon_t){ 8,3})
#define PH_ICON_BI_CYCLE_ARROWS  ((icon_t){ 9,3})
#define PH_ICON_EYE_CLOSED       ((icon_t){10,3})
#define PH_ICON_EYE_OPEN         ((icon_t){11,3})
#define PH_ICON_PAINT_BUCKET     ((icon_t){12,3})
#define PH_ICON_STAR_ASTERISK    ((icon_t){13,3})
#define PH_ICON_SMALL_BIN        ((icon_t){14,3})
#define PH_ICON_HOLLOW_GEAR      ((icon_t){15,3})

// #define                             ((icon_t){ 0,4})
#define PH_ICON_SMALL_WINDOW_AND_BUG   ((icon_t){ 1,4})
#define PH_ICON_LOGO_YOUTUBE           ((icon_t){ 2,4})
#define PH_ICON_LOGO_GITHUB            ((icon_t){ 3,4})
#define PH_ICON_LOGO_ITCH              ((icon_t){ 4,4})
#define PH_ICON_ITALIC_I               ((icon_t){ 5,4})
#define PH_ICON_HOLLOW_HOME            ((icon_t){ 6,4})
#define PH_ICON_HOLLOW_ISO_CUBE        ((icon_t){ 7,4})
#define PH_ICON_HOLLOW_CODE            ((icon_t){ 8,4})
#define PH_ICON_FLOPPY_DISC            ((icon_t){ 9,4})
#define PH_ICON_HOLLOW_USER_PROFILE    ((icon_t){10,4})
#define PH_ICON_PENCIL                 ((icon_t){11,4})
// #define                             ((icon_t){12,4})
// #define                             ((icon_t){13,4})
// #define                             ((icon_t){14,4})
#define PH_ICON_OPAQUE_GEAR            ((icon_t){15,4})

// #define                             ((icon_t){ 0,5})
#define PH_ICON_LOGO_BIG_SHADER_LOOM   ((icon_t){ 1,5})
#define PH_ICON_LOGO_MID_SHADER_LOOM   ((icon_t){ 2,5})
#define PH_ICON_LOGO_SMALL_SHADER_LOOM ((icon_t){ 3,5})
// #define                             ((icon_t){ 4,5})
#define PH_ICON_BIG_ITALIC_I           ((icon_t){ 5,5})
#define PH_ICON_OPAQUE_HOME            ((icon_t){ 6,5})
#define PH_ICON_OPAQUE_ISO_CUBE        ((icon_t){ 7,5})
#define PH_ICON_OPAQUE_CODE            ((icon_t){ 8,5})
// #define                             ((icon_t){ 9,5})
#define PH_OPAQUE_ICON_USER_PROFILE    ((icon_t){10,5})
// #define                             ((icon_t){11,5})
// #define                             ((icon_t){12,5})
// #define                             ((icon_t){13,5})
// #define                             ((icon_t){14,5})
// #define                             ((icon_t){15,5})

// #define                         ((icon_t){ 0,6})
#define PH_ICON_NE_ARROW           ((icon_t){ 1,6})
#define PH_ICON_NE_SHRINK          ((icon_t){ 2,6})
#define PH_ICON_SW_GROW            ((icon_t){ 3,6})
#define PH_ICON_NE_SW_BI_ARROW     ((icon_t){ 4,6})
#define PH_ICON_PICTURE            ((icon_t){ 5,6})
#define PH_ICON_SURROUNDED_CORNERS ((icon_t){ 6,6})
#define PH_ICON_DEBUG_PLAY         ((icon_t){ 7,6})
#define PH_ICON_SMALL_PLAY         ((icon_t){ 8,6})
#define PH_ICON_EMPTY_SQUARE       ((icon_t){ 9,6})
#define PH_ICON_CHECK_SQUARE       ((icon_t){10,6})
#define PH_ICON_VARIABLE           ((icon_t){11,6})
#define PH_ICON_CASE_SENSITIVE     ((icon_t){12,6})
#define PH_ICON_REGEX              ((icon_t){13,6})
#define PH_ICON_PICTURE_NE_ARROW   ((icon_t){14,6})
// #define                         ((icon_t){15,6})

// #define                  ((icon_t){ 0,7})

// #define                       ((icon_t){ 0,8})
#define PH_ICON_PLUS              ((icon_t){ 1,8})
#define PH_ICON_AERO_STICKY_NOTE  ((icon_t){ 2,8})
#define PH_ICON_AERO_GEAR         ((icon_t){ 3,8})
#define PH_ICON_AERO_WINDOW_ROUND ((icon_t){ 4,8})
#define PH_ICON_AERO_LADY_BIRD    ((icon_t){ 5,8})
#define PH_ICON_AERO_WINDOW_NOTE  ((icon_t){ 6,8})

#define PH_ICON_RED_SWITCH_OFF    ((icon_t){ 9,7})
#define PH_ICON_RED_SWITCH_ON     ((icon_t){ 9,8})
#define PH_ICON_GREEN_SWITCH_OFF  ((icon_t){10,7})
#define PH_ICON_GREEN_SWITCH_ON   ((icon_t){10,8})

#pragma endregion

#pragma region GUI MEASURES

typedef struct {
	double guiScale;
	double iconSize;
} phguiMeasures_t;

phguiMeasures_t phguiMeasures();
phguiMeasures_t phguiUncomputedMeasures();

#pragma endregion


#pragma region WIN.C

typedef struct GLFWwindow GLFWwindow;

// typedef struct ivec2_t icon_t;

typedef struct {
	GLFWwindow *glfwPtr;
	string_t    title;
	int width, height, targetFPS;

	double frameStartTime, deltaTime, time;
	size_t frameCounter;

	FT_Library freetypeLibrary;

	void (*onResize)(int, int);

	struct {
		struct {
			matrix_t model, view, projection;
			matrix_t m4x4identity;
		} matrices;

		mesh_ptr    quadMesh;
		texture_ptr blankTex, checkerTex, iconsTex;
		font_ptr    internalFont;
		shader_ptr  basicShader, textShader, rectangleShader;
	} primitives;

	struct {
		void* arena;
		Clay_RenderCommandArray renderCommands;

		void*  textRenderInstructions;
		size_t textInstructionsLen;

		phguiMeasures_t measures;
	} clay;

	struct {
		texture_ptr firstPass;
	} renderPasses;

} window_t;

// must define `onResize(win, w, h)` in program
error_t phInitWindow(c_string_t title, int width, int height, int fps);

// updates the window, ends the last frame & begins the next. Then checks if the window should close
bool phWindowShouldClose();

void phBeginDrawing();
void phEndDrawing();

void phCloseWindow();

void phSetClearColour(colour_t c);
// void phSetViewport(int width, int height);

void   phSetOnResizeFunction(void (*onResize)(int, int));

int       phGetWinWidth();
int       phGetWinHeight();
double    phGetDeltaTime();
double    phGetTime();
window_t* phGetWindow();
int       phGetFrame();

#pragma endregion

#pragma region COLOUR

#define PH_COLOUR_TRANSPARENT clayRGB(0xfff0)
#define PH_COLOUR_BLANK       PH_COLOUR_TRANSPARENT

#define PH_COLOUR_BACKGROUND clayRGB(0x1f1f1f)
#define PH_COLOUR_SIDE_PANEL clayRGB(0x242424)
#define PH_COLOUR_SIDE_PANEL_SETTINGS clayRGB(0x323232)
#define PH_COLOUR_OUTLINE    clayRGB(0x484848)
#define PH_COLOUR_ACCENT_A   clayRGB(0xf08)

#define PH_COLOUR_ELEMENT_BG_A clayRGB(0x363636)
#define PH_COLOUR_ELEMENT_BG_B PH_COLOUR_OUTLINE

#define PH_COLOUR_ELEMENT_FG clayRGB(0xf7c)

#define PH_PADDING_A 6
#define PH_PADDING_B 3

#define PH_VARIABLE_TYPE_COLOUR clayRGB(0xf92672)

#define PH_COLOUR_NOT_ALLOWED_RED clayRGB(0xdc143c)

// #define PH_COLOUR_TRANSPARENT clayRGB(0xfff0)
// #define PH_COLOUR_BLANK       PH_COLOUR_TRANSPARENT

// #define PH_COLOUR_BACKGROUND clayRGB(0x1a1a1a)
// #define PH_COLOUR_SIDE_PANEL clayRGB(0x242424)
// #define PH_COLOUR_OUTLINE    clayRGB(0x484848)
// #define PH_COLOUR_ACCENT_A   clayRGB(0xf08)

// #define PH_COLOUR_ELEMENT_BG_A clayRGB(0x363636)
// #define PH_COLOUR_ELEMENT_BG_B PH_COLOUR_OUTLINE

// #define PH_COLOUR_ELEMENT_FG clayRGB(0xf7c)

// #define PH_PADDING_A 6
// #define PH_PADDING_B 3

// #define PH_VARIABLE_TYPE_COLOUR clayRGB(0xf92672)

#pragma endregion

#endif