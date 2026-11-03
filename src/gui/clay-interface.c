#ifdef INTELLISENSE
	#define GLAPI
	#define APIENTRY
	#define GL_GLEXT_PROTOTYPES
	#include <glad/glcorearb.h>
#endif
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#include <memory.h>
#include <maths.h>

#define PHOTON_FEATURE__OPENGL_STACK_OPERATIONS
#include "../photon.h"

#define CLAY_IMPLEMENTATION
#include <clay/clay.h>

// typedef struct {
// 	scissor_t scissor;
// 	size_t startingElementIndex;
// 	size_t elementCount;
// } scissorTextBatch_t;

// typedef struct {
// 	int    startingElementIndex;
// 	int    endingElementIndex;

// 	size_t elementCount;
// 	size_t scissorBatchCount;
	
// 	font_ptr font;

// 	void* scissors;

// } textElementMetadata_t;
typedef struct {
	c_string_t str;
	size_t  strlen;
	ivec2_t    pos;

	colour_t     col;
	int32_t fontSize;
} textRenderElement_t;

typedef enum {
	TEXT_INS_INITIATOR,
	TEXT_INS_TEXT_ELEMENT,
	TEXT_INS_START_SCISSOR,
	TEXT_INS_END_SCISSOR,
} textInstructionType_e;

typedef struct {
	textInstructionType_e  type;
	size_t nextInstructionIndex;

	union {
		textRenderElement_t element;
		scissor_t           scissor;
	};
} textRenderInstruction_t;

typedef struct {
	size_t   firstInstructionIndex;
	size_t   lastInstructionIndex;
	font_ptr font;
} textInstructionBeginner_t;


void drawTextElements(textInstructionBeginner_t* beginners);

void clayErrorHandler(Clay_ErrorData errorData) {
	// See the Clay_ErrorData struct for more information
	printf("%s\n", errorData.errorText.chars);
}


Clay_Dimensions clayMeasureText(Clay_StringSlice text, Clay_TextElementConfig *config, void *userData) {
	extern addressCache_t fontCache;

	font_ptr font =  (font_ptr)fontCache.addresses[config->fontId];
	ivec2_t size = phMeasureText(font, text.chars, text.length, config->fontSize, config->letterSpacing + 1, config->lineHeight);
	
	return (Clay_Dimensions){
		.width = size.x,
		.height = size.y
	};
	// font_ptr font = (font_ptr)(activeFontStack->basePtr + (((size_t)config->fontId) * sizeof(font_ptr)));
	// ivec2 size = measureText(font, text.chars, text.length, config->fontSize, config->letterSpacing + 1, config->lineHeight);
	
	// return (Clay_Dimensions){
	// 	.width = size.x,
	// 	.height = size.y
	// };
}

void initMeasures();

void phguiInit() {
	window_t* win = phGetWindow();
	extern size_t genIDIndex;

	genIDIndex = 0;

	uint64_t totalMemorySize = Clay_MinMemorySize();
	win->clay.arena = memalloc(Clay_MinMemorySize());
	initMeasures();
	Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, win->clay.arena);

	Clay_Initialize(arena, (Clay_Dimensions) { phGetWinWidth(), phGetWinHeight() }, (Clay_ErrorHandler) { clayErrorHandler });
	Clay_SetMeasureTextFunction(clayMeasureText, NULL);

	// Clay_SetDebugModeEnabled(true);

	win->clay.textRenderInstructions = memalloc(sizeof(textRenderInstruction_t) * (PH_FONT_CACHE_LEN + PH_TEXT_RENDERER_MAX_TEXT_ELEMENTS + PH_MAX_SCISSORS_PER_FRAME * 2));
}
void phguiClose() {
	window_t* win = phGetWindow();
	free(win->clay.arena);
	free(win->clay.textRenderInstructions);

	// for (size_t i = 0; i < PH_FONT_CACHE_LEN; i++) {
	// 	photonWindow.clay.scissorTextBatches[i] = memalloc(PH_MAX_SCISSORS_PER_FRAME * 2 * sizeof(scissorTextBatch_t));
	// }
}
void phguiBeginLayout() {
	Clay_SetLayoutDimensions((Clay_Dimensions) { phGetWinWidth(), phGetWinHeight() });
	// printf("size: %d, %d\n", phGetWinWidth(), phGetWinHeight());
	dvec2_t mousePos = phGetMousePosition();
	dvec2_t mouseWheel = phGetMouseWheelDelta();
	bool isLeftMouseDown = phIsMouseButton(GLFW_MOUSE_BUTTON_LEFT, MouseDown);
	Clay_SetPointerState((Clay_Vector2) {mousePos.x, mousePos.y}, isLeftMouseDown);
	
	Clay_UpdateScrollContainers(false, (Clay_Vector2) {mouseWheel.x * 2, mouseWheel.y * 2}, phGetDeltaTime());

	Clay_BeginLayout();
}

void phguiEndLayout() {
	phGetWindow()->clay.renderCommands = Clay_EndLayout();
}

void drawRectangle(irec32_t rec, fvec4_t cornerRadii, fvec4_t borderWidths, colour_t backgroundColour, colour_t borderColour) {
	window_t* win = phGetWindow();
	mesh_ptr m    = phGLPeekMesh();
	texture_ptr t = phGLPeekTexture(0);

	phSetMatrixToRec(&win->primitives.matrices.model, rec);

	phSetUniformf("model",      'm4x4', win->primitives.matrices.model);
	phSetUniformf("uSize",      'v2',  (double)rec.w, (double)rec.h);
	phSetUniformf("uCurvature", 'v4', 
		(double)cornerRadii.x, (double)cornerRadii.y,
		(double)cornerRadii.z, (double)cornerRadii.w
	);
	phSetUniformf("uBorderWidth", 'v4',
		(double)borderWidths.x, (double)borderWidths.y,
		(double)borderWidths.z, (double)borderWidths.w
	);
	phSetUniformf("uColour",       'rgba', backgroundColour);
	phSetUniformf("uBorderColour", 'rgba', borderColour);
	phSetUniformf("uBurnColour",   'rgba', rgb(0xfff0));
	phSetUniformf("uUVRectangle",  'v4',   0, 0, t->width, t->height);
	glDrawElements(GL_TRIANGLES, m->indicesLen, GL_UNSIGNED_SHORT, NULL);
}

void drawImage(irec32_t rec, frec_t uvRec, fvec4_t cornerRadii, colour_t backgroundColour, colour_t burnColour) {
	window_t* win = phGetWindow();
	mesh_ptr m = phGLPeekMesh();

	phSetMatrixToRec(&win->primitives.matrices.model, rec);

	phSetUniformf("model",      'm4x4', win->primitives.matrices.model);
	phSetUniformf("uSize",      'v2',  (double)rec.w, (double)rec.h);
	phSetUniformf("uCurvature", 'v4', 
		(double)cornerRadii.x, (double)cornerRadii.y,
		(double)cornerRadii.z, (double)cornerRadii.w
	);
	phSetUniformf("uBorderWidth",  'v4',   0.0, 0.0, 0.0, 0.0);
	phSetUniformf("uColour",       'rgba', backgroundColour);
	phSetUniformf("uBorderColour", 'rgba', rgb(0xfff0));
	phSetUniformf("uBurnColour",   'rgba', burnColour);
	phSetUniformf("uUVRectangle",  'v4',
		(double)uvRec.x, (double)uvRec.y,
		(double)uvRec.w, (double)uvRec.h
	);

	glDrawElements(GL_TRIANGLES, m->indicesLen, GL_UNSIGNED_SHORT, NULL);
}

fvec2_t mousePosRelativeToShaderArea;

void phguiRender() {
	// extern matrix_t model;
	// extern matrix_t view;
	// extern matrix_t projection;
	extern addressCache_t fontCache;

	window_t* win = phGetWindow();

	Clay_RenderCommandArray commands = win->clay.renderCommands;

	phGLPushScissor((scissor_t){0,0,win->width, win->height});

	// metadata stored per font
	textRenderInstruction_t*  textRenderInstructions = win->clay.textRenderInstructions;
	textInstructionBeginner_t textInstructionBeginners[PH_FONT_CACHE_LEN];
	win->clay.textInstructionsLen = 0;
	foreachAddress(fontCache, i) {
		textInstructionBeginners[i] = (textInstructionBeginner_t){
			.firstInstructionIndex = win->clay.textInstructionsLen,
			.lastInstructionIndex  = win->clay.textInstructionsLen,
			.font = fontCache.addresses[i],
		};
		textRenderInstructions[win->clay.textInstructionsLen++] = (textRenderInstruction_t){
			.type = TEXT_INS_INITIATOR,
		};
	}


	phGLPushShader (   win->primitives.rectangleShader);
	phGLPushMesh   (   win->primitives.quadMesh       );
	phGLPushTexture(0, win->primitives.blankTex       );
	
	phSetUniformf("view",       'm4x4', win->primitives.matrices.view);
	phSetUniformf("projection", 'm4x4', win->primitives.matrices.projection);


	glBindFramebuffer(GL_FRAMEBUFFER, win->renderPasses.firstPass->ren.framebuffer);
	glClear(GL_COLOR_BUFFER_BIT);


	for (int i = 0; i < commands.length; i++) {
		Clay_RenderCommand cmd = commands.internalArray[i];
		irec32_t bb = (irec32_t){cmd.boundingBox.x,cmd.boundingBox.y,cmd.boundingBox.width,cmd.boundingBox.height};

		switch (cmd.commandType) {
		case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
			if (cmd.userData == (void*)1) {
				extern shader_ptr previewShader;
				Clay_RectangleRenderData renderData = cmd.renderData.rectangle;
				mesh_ptr m = phGLPeekMesh();
				colour_t backgroundColour = phClayColourToPhColour(renderData.backgroundColor);
				colour_t borderColour = backgroundColour; borderColour.a = 0;
				
				phGLPushShader(previewShader);
				
				phSetMatrixToRec(&win->primitives.matrices.model, bb);
				phSetUniformf("model",      'm4x4', win->primitives.matrices.model);
				phSetUniformf("view",       'm4x4', win->primitives.matrices.view);
				phSetUniformf("projection", 'm4x4', win->primitives.matrices.projection);

				phSetUniformf("iResolution", 'v3', (double)bb.w, (double)bb.h, 1.0);
				double time = phGetTime();
				phSetUniformf("iTime",       '1f',  time);
				// printf("%f\n", time);
				// phSetUniformf(c_string_t name, uint32_t fmt, ...)
				phSetUniformf("iTimeDelta",  '1f',  (double)phGetDeltaTime());
				phSetUniformf("iFrameRate",  '1f',  (double)(1./phGetDeltaTime()));
				phSetUniformf("iFrame",      'i',  (int)(phGetFrame()));
				// phSetUniformf("iDate",       'v4',  year, month, day, seconds);
				extern bool canPanAndZoomShader;
				if (!canPanAndZoomShader && phGetMousePosition().x >= bb.x && phGetMousePosition().y >= bb.y && phGetMousePosition().x <= bb.x+bb.w && phGetMousePosition().y <= bb.y+bb.h) {
					bool isDown, clicked;
					isDown =
						phIsMouseButton(GLFW_MOUSE_BUTTON_LEFT, MouseDown)   ||
						phIsMouseButton(GLFW_MOUSE_BUTTON_MIDDLE, MouseDown) ||
						phIsMouseButton(GLFW_MOUSE_BUTTON_RIGHT, MouseDown)  ;
					clicked =
						phIsMouseButton(GLFW_MOUSE_BUTTON_LEFT, MousePressed)   ||
						phIsMouseButton(GLFW_MOUSE_BUTTON_MIDDLE, MousePressed) ||
						phIsMouseButton(GLFW_MOUSE_BUTTON_RIGHT, MousePressed)  ;
					phSetUniformf("iMouse",      'v4', phGetMousePosition().x - bb.x, bb.h - (phGetMousePosition().y - bb.y), (double)isDown, (double)clicked);
					mousePosRelativeToShaderArea.x = (double)(phGetMousePosition().x - bb.x) / (double)bb.w;
					mousePosRelativeToShaderArea.y = (double)(bb.h - (phGetMousePosition().y - bb.y)) / (double)bb.h;
				}
				// phSetUniformf("uColour",       'rgba', backgroundColour);
				glDrawElements(GL_TRIANGLES, m->indicesLen, GL_UNSIGNED_SHORT, NULL);
				// drawRectangle(
				// 	bb,
				// 	(fvec4_t){
				// 		renderData.cornerRadius.topLeft,  renderData.cornerRadius.bottomLeft,
				// 		renderData.cornerRadius.topRight, renderData.cornerRadius.bottomRight
				// 	},
				// 	(fvec4_t){0, 0, 0, 0},
				// 	backgroundColour,
				// 	borderColour
				// );
				phGLPopShader();
				break;
			} 
			Clay_RectangleRenderData renderData = cmd.renderData.rectangle;
			mesh_ptr m = phGLPeekMesh();
			colour_t backgroundColour = phClayColourToPhColour(renderData.backgroundColor);
			colour_t borderColour = backgroundColour; borderColour.a = 0;

			drawRectangle(
				bb,
				(fvec4_t){
					renderData.cornerRadius.topLeft,  renderData.cornerRadius.bottomLeft,
					renderData.cornerRadius.topRight, renderData.cornerRadius.bottomRight
				},
				(fvec4_t){0, 0, 0, 0},
				backgroundColour,
				borderColour
			);
		} break;
		case CLAY_RENDER_COMMAND_TYPE_BORDER: {
			Clay_BorderRenderData renderData = cmd.renderData.border;
			mesh_ptr m = phGLPeekMesh();
			colour_t borderColour = phClayColourToPhColour(renderData.color);
			colour_t backgroundColour = borderColour; backgroundColour.a = 0;

			drawRectangle(
				bb,
				(fvec4_t){
					renderData.cornerRadius.topLeft,  renderData.cornerRadius.bottomLeft,
					renderData.cornerRadius.topRight, renderData.cornerRadius.bottomRight
				},
				(fvec4_t){
					(double)renderData.width.top,   (double)renderData.width.left,
					(double)renderData.width.right, (double)renderData.width.bottom
				},
				backgroundColour,
				borderColour
			);

		} break;

		case CLAY_RENDER_COMMAND_TYPE_TEXT: {
			Clay_TextRenderData renderData = cmd.renderData.text;

			textRenderInstructions[win->clay.textInstructionsLen] = (textRenderInstruction_t){
				.nextInstructionIndex = 0,
				.type = TEXT_INS_TEXT_ELEMENT,

				.element.str    = renderData.stringContents.chars,
				.element.strlen = renderData.stringContents.length,
				.element.pos      = (ivec2_t){bb.x, bb.y},
				.element.col      = phClayColourToPhColour(renderData.textColor),
				.element.fontSize = renderData.fontSize,
			};
			textRenderInstructions[textInstructionBeginners[renderData.fontId].lastInstructionIndex].nextInstructionIndex = win->clay.textInstructionsLen;
			textInstructionBeginners[renderData.fontId].lastInstructionIndex = win->clay.textInstructionsLen;
			win->clay.textInstructionsLen++;
		} break;

		case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
			scissor_t scissor = (scissor_t){bb.x, bb.y, bb.w, bb.h};
			phGLSetScissor(scissor);

			foreachAddress(fontCache, fontId) {
				textRenderInstructions[win->clay.textInstructionsLen] = (textRenderInstruction_t){
					.nextInstructionIndex = 0,
					.type = TEXT_INS_START_SCISSOR,

					.scissor = scissor,
				};
				textRenderInstructions[textInstructionBeginners[fontId].lastInstructionIndex].nextInstructionIndex = win->clay.textInstructionsLen;
				textInstructionBeginners[fontId].lastInstructionIndex = win->clay.textInstructionsLen;
				win->clay.textInstructionsLen++;
			}
		} break;
		case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
			phGLSetScissor((scissor_t){0,0, win->width, win->height});
			foreachAddress(fontCache, fontId) {
				if (textRenderInstructions[textInstructionBeginners[fontId].lastInstructionIndex].type == TEXT_INS_START_SCISSOR) {
					win->clay.textInstructionsLen--;
					textRenderInstructions[textInstructionBeginners[fontId].lastInstructionIndex].nextInstructionIndex = 0;
					textInstructionBeginners[fontId].lastInstructionIndex = win->clay.textInstructionsLen;
				}
				else {
					textRenderInstructions[win->clay.textInstructionsLen] = (textRenderInstruction_t){
						.nextInstructionIndex = 0,
						.type = TEXT_INS_END_SCISSOR,
					};
					textRenderInstructions[textInstructionBeginners[fontId].lastInstructionIndex].nextInstructionIndex = win->clay.textInstructionsLen;
					textInstructionBeginners[fontId].lastInstructionIndex = win->clay.textInstructionsLen;
					win->clay.textInstructionsLen++;
				}
			}
		} break;

		case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
			Clay_ImageRenderData renderData = cmd.renderData.image;
			elementMetadata_ptr  metadata   = (elementMetadata_ptr)renderData.imageData;
			if (metadata->type == PH_META_ELE_ICON) {
				mesh_ptr m = phGLPeekMesh();
				phGLPushTexture(0, win->primitives.iconsTex);
				drawImage(
					bb,
					(frec_t) {metadata->icon.x * 48, metadata->icon.y * 48, 48, 48},
					(fvec4_t){
						renderData.cornerRadius.topLeft,  renderData.cornerRadius.bottomLeft,
						renderData.cornerRadius.topRight, renderData.cornerRadius.bottomRight
					},
					phClayColourToPhColour(renderData.backgroundColor), rgb(0xfff0)
				);
				phGLPopTexture(0);
			}
			else if (metadata->type == PH_META_ELE_9_SLICED_ICON) {
				mesh_ptr m = phGLPeekMesh();
				phGLPushTexture(0, win->primitives.iconsTex);
				irec32_t bbs[9] = {
					{bb.x, bb.y        , 23, 23     }, {bb.x+23, bb.y        , bb.w-46, 23     }, {bb.x+bb.w-23, bb.y        , 23, 23     },
					{bb.x, bb.y     +23, 23, bb.h-46}, {bb.x+23, bb.y     +23, bb.w-46, bb.h-46}, {bb.x+bb.w-23, bb.y     +23, 23, bb.h-46},
					{bb.x, bb.y+bb.h-23, 23, 23     }, {bb.x+23, bb.y+bb.h-23, bb.w-46, 23,    }, {bb.x+bb.w-23, bb.y+bb.h-23, 23, 23     },
				};
				frec_t uvRecs[9] = {
					{ 0, 0,23,23}, {23, 0, 2,23}, {25, 0,23,23},
					{ 0,23,23, 2}, {23,23, 2, 2}, {25,23,23, 2},
					{ 0,25,23,23}, {23,25, 2,23}, {25,25,23,23},
				};
				colour_t cols[9] = {
					rgb(0xfff), rgb(0xf00), rgb(0x0f0),
					rgb(0x888), rgb(0x00f), rgb(0xff0),
					rgb(0x000), rgb(0x0ff), rgb(0xf0f),
				};
				for (size_t i = 0; i < 9; i++) {
					drawImage(
						bbs[i],
						(frec_t) {
							metadata->nineSlicedIcon.x * 48 + uvRecs[i].x,
							metadata->nineSlicedIcon.y * 48 + uvRecs[i].y,
							uvRecs[i].w, uvRecs[i].h},
						(fvec4_t){
							renderData.cornerRadius.topLeft * (i==0), renderData.cornerRadius.topRight * (i==2),
							renderData.cornerRadius.bottomLeft * (i==6), renderData.cornerRadius.bottomRight * (i==8)
						},
						phClayColourToPhColour(renderData.backgroundColor), metadata->nineSlicedIcon.burn
					);
				}
				phGLPopTexture(0);
			}

		} break;

		case CLAY_RENDER_COMMAND_TYPE_CUSTOM: {
		} break;

		default: case CLAY_RENDER_COMMAND_TYPE_NONE:
		}
	}
	phGLPopScissor();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	// final render pass
	phGLPushShader(win->primitives.basicShader);
	phGLPushScissor((scissor_t){0,0,win->width, win->height});
	phGLPushTexture(0, win->renderPasses.firstPass);
		phSetMatrixToRec(&win->primitives.matrices.model, (irec32_t){
			0, phGetWinHeight(),
			phGetWinWidth(), -phGetWinHeight()
		});
		phSetUniformf("model",      'm4x4', win->primitives.matrices.model);
		phSetUniformf("view",       'm4x4', win->primitives.matrices.view);
		phSetUniformf("projection", 'm4x4', win->primitives.matrices.projection);
		phSetUniformf("uColour",    'rgba', rgb(0xfff));
		mesh_ptr m = phGLPeekMesh();
		glDrawElements(GL_TRIANGLES, m->indicesLen, GL_UNSIGNED_SHORT, NULL);
	phGLPopTexture(0);
	phGLPopScissor( );
	phGLPopShader ( );

	// draw all cleartype text
	phGLPushScissor((scissor_t){0,0,win->width, win->height});
	phGLPushShader(win->primitives.textShader);
	phGLPushTexture(2, win->renderPasses.firstPass);
		drawTextElements(textInstructionBeginners);
	phGLPopTexture(2);
	phGLPopShader();
	phGLPopScissor( );
	
	phGLPopTexture(0);
	phGLPopMesh   ( );
	phGLPopShader ( );
}

void drawFontMesh(mesh_ptr m, size_t glyphCount) {
	// extern matrix_t model;
	// extern matrix_t view;
	// extern matrix_t projection;
	window_t* win = phGetWindow();
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(fontMeshVertex_t) * glyphCount * 4,  m->vertices);

	phGLPushMesh(m);
	
	phSetUniformf("view",       'm4x4', win->primitives.matrices.view);
	phSetUniformf("projection", 'm4x4', win->primitives.matrices.projection);
	
	glDrawElements(GL_TRIANGLES, m->indicesLen * glyphCount, GL_UNSIGNED_SHORT, NULL);

	phGLPopMesh();
}

void drawTextElement(font_ptr f, textRenderElement_t element) {
	mesh_ptr m = f->renderer->batchedMesh;
	fontMeshVertex_t* vertices = (fontMeshVertex_t*)m->vertices;
	const size_t maxGlyphCount  = PH_FONT_RENDERER_MAX_GLYPH_INSTANCES;
	const size_t vsize = sizeof(fontMeshVertex_t) * maxGlyphCount * 4;
	
	size_t i;
	
	for (i = 0; i < element.strlen; i++) {
		char c = element.str[i] - ' ';
		
		if (i >= maxGlyphCount) {
			// render and reset
		}
		for (size_t j=0; j<4; j++) {
			vertices[i*4 + j] = f->renderer->vertices[c * 4 + j];

			vertices[i*4 + j].x += element.pos.x;
			vertices[i*4 + j].y -= element.pos.y;

			vertices[i*4 + j].r = (float)element.col.r / 255.0;
			vertices[i*4 + j].g = (float)element.col.g / 255.0;
			vertices[i*4 + j].b = (float)element.col.b / 255.0;
		}
		element.pos.x += phRound((double)f->glyphs[c].advance.x / 64.0, 1);
	}
	drawFontMesh(m, i);
}

void drawTextElements(textInstructionBeginner_t* beginners) {
	window_t* win = phGetWindow();
	extern addressCache_t fontCache;
	
	textRenderInstruction_t* instructions = win->clay.textRenderInstructions;

	foreachAddress(fontCache, i) {
		textRenderInstruction_t ins = instructions[beginners[i].firstInstructionIndex];

		phGLPushTexture(1, beginners[i].font->texture);
		glBindBuffer(GL_ARRAY_BUFFER, beginners[i].font->renderer->batchedMesh->VBO);

		do {
			ins = instructions[ins.nextInstructionIndex];
			switch (ins.type) {
			case TEXT_INS_TEXT_ELEMENT: {
				drawTextElement(beginners[i].font, ins.element);
			} break;
			case TEXT_INS_START_SCISSOR: {
				phGLSetScissor(ins.scissor);
			} break;
			case TEXT_INS_END_SCISSOR: {
				phGLSetScissor((scissor_t){0,0, win->width, win->height});
			} break;
			case TEXT_INS_INITIATOR: default:
			}
		} while (ins.nextInstructionIndex != 0);
		
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		phGLPopTexture(1);
	}
}